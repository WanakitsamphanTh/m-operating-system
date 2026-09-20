#include <climits>
#include <cstdint>
#include <cstring>
#include <kernel_core.hpp>
#include <kernel/dtb.hpp>
#include <kernel/io.hpp>
#include "kernel/mem/region.hpp"
#include "kernel/mem/page.hpp"
#include "kernel/mem/allocator.hpp"
#include "mstd/monadic/maybe.hpp"
#include "mstd/scope_guard.hpp"
#include "mstd/string.hpp"
#include "mstd/mem/alloc.hpp"
#include "kernel/mem/manager.hpp"

#include <cstddef>
#include <utility>

using mstd::maybe;
using mstd::nothing;
using mstd::some;
using mstd::as_ptr;

alignas(MK::page_size) MK::PageDescriptor l0_table[512];

using MK::Bootstrap;
using MK::Permanent;

/* these only work in the higher half kernel */
extern MK::PageManager<Permanent> page_manager;
extern MK::KernelConsole console;

extern "C" [[noreturn]] int kernel_bootstrap(uint8_t* dtb){
    MK::PageManager<Bootstrap> page_manager;
    auto& page_allocator = page_manager.get_allocator();
    auto& regions = page_allocator.get_regions();

    printk("start kmain\n");

    // parse device tree blob
    auto fdt = MK::FDT::try_read_fdt(dtb).take("cant parse dtb");
    const auto& root = fdt.find_node("").take("cant find root");
    const auto& mem = root.find_node_prefix("memory").take("cant find memory");
    const auto& gicd = fdt.find_compatible("arm,gic-v3").take(" cant find gicd");
    const auto& uart = fdt.find_compatible("arm,pl011").take("cant find uart");
    const auto& vit = fdt.find_compatible("virtio,mmio").take("cant find vit");
    const auto fdt_size = 4 * 9 + fdt.string_size + fdt.struct_size;
    fdt.print(console);

    // init uart
    uart.find_property("reg", fdt).then_or_else([](const MK::FDTProperty& prop){
        ::uart_base = uintptr_t(prop.u32_at(0).take()) << 32 | uintptr_t(prop.u32_at(4).take());
        ::uart_size = uint64_t(prop.u32_at(8).take()) << 32 | uint64_t(prop.u32_at(12).take());
    }, []() __attribute__((noreturn)) {
        printk("Cant find property reg in uart");
        kernel_panic();
    });

    printk("initialized UART");

    gicd.find_property("reg", fdt).then_or_else([&](const MK::FDTProperty& prop){
        ::gicd_base = uintptr_t(prop.u32_at(0).take()) << 32 | uintptr_t(prop.u32_at(4).take());
        ::gicd_size = uint64_t(prop.u32_at(8).take()) << 32 | uint64_t(prop.u32_at(12).take());
        ::gicr_base = uintptr_t(prop.u32_at(16).take()) << 32 | uintptr_t(prop.u32_at(20).take());
        ::gicr_size = uint64_t(prop.u32_at(24).take()) << 32 | uint64_t(prop.u32_at(28).take());
    }, []() __attribute__((noreturn)){
        printk("Cant find property reg in gicd");
        kernel_panic();
    });

    /* init memory */
    const auto size_cells = root.find_property("#size-cells", fdt).take().u32_at(0).take();
    const auto addr_cells = root.find_property("#address-cells", fdt).take().u32_at(0).take();

    const auto& mem_reg = mem.find_property("reg", fdt).take("cannot find the property reg");
    auto entry_size = (addr_cells + size_cells) * 4;
    regions.num =  mem_reg.len / entry_size;
    regions.num = regions.num < 8 ? regions.num : 8;

    printk("number of banks %d\n", regions.num);
    for(size_t i = 0; i < regions.num; i++){
        size_t off = i * entry_size;
        auto& span = regions[i].get_span();
        span.base = 
            uint64_t(mem_reg.u32_at(off).take()) << 32
            | uint64_t(mem_reg.u32_at(off + 4).take());
        span.size =
            uint64_t(mem_reg.u32_at(8 + off).take()) << 32
            | uint64_t(mem_reg.u32_at(12 + off).take());
        printk("regions[%d] base %d size %d\n", i, span.base, span.size);
    }    

    page_allocator.init(
        MK::PhySpan{_kernel_start_addr, _kernel_size},
        MK::PhySpan{reinterpret_cast<uintptr_t>(fdt.base_ptr), fdt_size}
    );
    printk("init page allocator\n");

    page_manager.init(l0_table);
    printk("init page manager\n");
    page_manager.map(_kernel_start_addr, _kernel_rodata_end_addr - _kernel_start_addr, MK::MapMode::Id, MK::PageInfo::MemoryType::Normal, MK::PageInfo::AP::PRO);
    printk("id map kernel\n");
    page_manager.map(_kernel_data_start_addr, _kernel_end_addr - _kernel_data_start_addr, MK::MapMode::Id, MK::PageInfo::MemoryType::Normal, MK::PageInfo::AP::PRW);
    printk("id map data/bss/stack\n");
    page_manager.map(reinterpret_cast<uintptr_t>(dtb), fdt_size, MK::MapMode::Id, MK::PageInfo::MemoryType::Normal, MK::PageInfo::AP::PRO);
    printk("id map FDT\n");
    page_manager.map(::uart_base, ::uart_size, MK::MapMode::Id, MK::PageInfo::MemoryType::Device, MK::PageInfo::AP::PRW);
    printk("id map UART\n");
    page_manager.map(::gicd_base, ::gicd_size, MK::MapMode::Id, MK::PageInfo::MemoryType::Device, MK::PageInfo::AP::PRW);
    printk("id map GICD\n");
    page_manager.map(::gicr_base, ::gicr_size, MK::MapMode::Id, MK::PageInfo::MemoryType::Device, MK::PageInfo::AP::PRW);
    printk("id map GICR\n");

    for(auto i = 0; i < regions.num; i++){
        auto& span = regions[i].get_span();
        uintptr_t virt_addr = MK::phy2virt_as<uintptr_t>(span.base);
        auto size = span.size;
        page_manager.map(virt_addr, size, MK::MapMode::Direct, MK::PageInfo::MemoryType::Normal, MK::PageInfo::AP::PRW);
    }
    printk("direct map physical addresses\n");

    ::page_manager = std::move(page_manager).enable_mmu_and_relocate();

    printk("enabled MMU");
    printk("this should work after MMU is enabled\n");

    /*===================== from now, everything that needs the higher half address works /*=====================*/

    /* relocation */
    ::uart_base = MK::phy2kvirt_as<uintptr_t>(uart_base);
    ::gicd_base = MK::phy2kvirt_as<uintptr_t>(gicd_base);
    ::gicr_base = MK::phy2kvirt_as<uintptr_t>(gicr_base);
    auto dt = std::move(fdt).relocate();

    printk("This should work after relocation\n");

    MK::PhySpan unmap_spans[] = {
        MK::PhySpan{ _kernel_start_addr, _kernel_rodata_end_addr - _kernel_start_addr },
        MK::PhySpan{ _kernel_data_start_addr, _kernel_end_addr - _kernel_data_start_addr },
        MK::PhySpan{ reinterpret_cast<uintptr_t>(dtb), fdt_size },
        MK::PhySpan{ ::uart_base, ::uart_size },
        MK::PhySpan{ ::gicd_base, ::gicd_size },
        MK::PhySpan{ ::gicr_base, ::gicr_size },
    };
    auto unmap_count = sizeof(unmap_spans) / sizeof(MK::PhySpan);
    MK::PhySpan* hi_unmap_spans = MK::phy2kvirt_as<MK::PhySpan*>(reinterpret_cast<uintptr_t>(unmap_spans));

    uintptr_t cur_sp;
    __asm__ __volatile__(
        "mov %0, sp\n"
        : "=r"(cur_sp)
    );
    uintptr_t hi_sp = MK::phy2kvirt_as<uintptr_t>(cur_sp);

    console.writef("switched stack address to higher half 0x{016h}\n", hi_sp);
    uintptr_t hi_kernel_main = MK::phy2kvirt_as<uintptr_t>(reinterpret_cast<uintptr_t>(&kernel_main));

    __asm__ __volatile__ (
        "mov sp, %0\n"
        "mov x1, %1\n"
        "br x1\n"
        :
        : "r"(hi_sp), "r"(hi_kernel_main)
        : "x0", "x1", "memory"
    );
}