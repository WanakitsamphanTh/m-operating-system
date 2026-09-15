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

#include <cstddef>
#include <utility>

using mstd::maybe;
using mstd::nothing;
using mstd::some;
using mstd::as_ptr;

MK::KernelConsole console;
MK::Regions regions;
MK::PageAlloc page_allocator;
MK::PageTablesManager page_manager;

extern "C" [[noreturn]] int kernel_bootstrap(uint8_t* dtb){
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
        console.writeln("Cant find property reg in uart");
        kernel_panic();
    });

    console.writeln("initialized UART");

    gicd.find_property("reg", fdt).then_or_else([&](const MK::FDTProperty& prop){
        ::gicd_base = uintptr_t(prop.u32_at(0).take()) << 32 | uintptr_t(prop.u32_at(4).take());
        ::gicd_size = uint64_t(prop.u32_at(8).take()) << 32 | uint64_t(prop.u32_at(12).take());
        ::gicr_base = uintptr_t(prop.u32_at(16).take()) << 32 | uintptr_t(prop.u32_at(20).take());
        ::gicr_size = uint64_t(prop.u32_at(24).take()) << 32 | uint64_t(prop.u32_at(28).take());
        console.writef("gicd base : 0x{016h}\n", ::gicd_base);
    }, []() __attribute__((noreturn)){
        console.writeln("Cant find property reg in gicd");
        kernel_panic();
    });

    /* init memory */
    const auto size_cells = root.find_property("#size-cells", fdt).take().u32_at(0).take();
    const auto addr_cells = root.find_property("#address-cells", fdt).take().u32_at(0).take();

    const auto& mem_reg = mem.find_property("reg", fdt).take("cannot find the property reg");
    auto entry_size = (addr_cells + size_cells) * 4;
    regions.num =  mem_reg.len / entry_size;
    regions.num = regions.num < 8 ? regions.num : 8;

    console.writef("number of banks {}\n", regions.num);
    for(size_t i = 0; i < regions.num; i++){
        size_t off = i * entry_size;
        regions[i].span.base = 
            uint64_t(mem_reg.u32_at(off).take()) << 32
            | uint64_t(mem_reg.u32_at(off + 4).take());
        regions[i].span.size =
            uint64_t(mem_reg.u32_at(8 + off).take()) << 32
            | uint64_t(mem_reg.u32_at(12 + off).take());
        console.writef("regions[{}] base 0x{016h} size 0x{016h}\n", i, regions[i].span.base, regions[i].span.size);
    }    
    page_allocator.init(
        regions,
        MK::PhySpan{_kernel_start_addr, _kernel_size},
        MK::PhySpan{reinterpret_cast<uintptr_t>(fdt.base_ptr), fdt_size}
    );
    console.writeln("init page allocator");

    page_manager.init(page_allocator);
    console.writeln("init page manager");

    page_manager.map(_kernel_start_addr, _kernel_rodata_end_addr - _kernel_start_addr, MK::MapMode::Id, MK::PageInfo::MemoryType::Normal, MK::PageInfo::AP::PRO);
    console.writeln("id map kernel");

    page_manager.map(_kernel_data_start_addr, _kernel_end_addr - _kernel_data_start_addr, MK::MapMode::Id, MK::PageInfo::MemoryType::Normal, MK::PageInfo::AP::PRW);
    console.writeln("id map data/bss/stack");

    page_manager.map(reinterpret_cast<uintptr_t>(dtb), fdt_size, MK::MapMode::Id, MK::PageInfo::MemoryType::Normal, MK::PageInfo::AP::PRO);
    console.writeln("id map FDT");

    page_manager.map(::uart_base, ::uart_size, MK::MapMode::Id, MK::PageInfo::MemoryType::Device, MK::PageInfo::AP::PRW);
    console.writeln("id map UART");

    page_manager.map(::gicd_base, ::gicd_size, MK::MapMode::Id, MK::PageInfo::MemoryType::Device, MK::PageInfo::AP::PRW);
    console.writeln("id map GICD");

    page_manager.map(::gicr_base, ::gicr_size, MK::MapMode::Id, MK::PageInfo::MemoryType::Device, MK::PageInfo::AP::PRW);
    console.writeln("id map GICR");

    for(auto i = 0; i < regions.num; i++){
        uintptr_t virt_addr = MK::phy2virt_as<uintptr_t>(regions[i].span.base);
        auto size = regions[i].span.size;
        page_manager.map(virt_addr, size, MK::MapMode::Direct, MK::PageInfo::MemoryType::Normal, MK::PageInfo::AP::PRW);
    }
    console.writeln("direct map physical addresses");

    page_manager.enable_mmu();
    console.writeln("enabled MMU");
    console.writeln("this should work after MMU is enabled");

    /* relink */
    ::uart_base = MK::phy2kvirt_as<uintptr_t>(uart_base);
    ::gicd_base = MK::phy2kvirt_as<uintptr_t>(gicd_base);
    ::gicr_base = MK::phy2kvirt_as<uintptr_t>(gicr_base);

    auto& relinked_fdt = *MK::phy2kvirt_as<MK::FDT*>(reinterpret_cast<uintptr_t>(&fdt));
    auto& relinked_console = *MK::phy2kvirt_as<MK::KernelConsole*>(reinterpret_cast<uintptr_t>(&console));
    auto& relinked_regions = *MK::phy2kvirt_as<MK::Regions*>(reinterpret_cast<uintptr_t>(&regions));
    auto& relinked_page_allocator = *MK::phy2kvirt_as<MK::PageAlloc*>(reinterpret_cast<uintptr_t>(&page_allocator));
    auto& relinked_page_mananger = *MK::phy2kvirt_as<MK::PageTablesManager*>(reinterpret_cast<uintptr_t>(&page_manager));

    page_allocator.relink(relinked_regions);
    page_manager.relink(relinked_page_allocator);
    for(auto i = 0; i < regions.num; i++)
        regions[i].relink([](uint8_t* ptr) -> uint8_t* {
            return MK::phy2kvirt_as<uint8_t*>(reinterpret_cast<uintptr_t>(ptr));
        });
    fdt.relink(
        MK::phy2kvirt_as<uint8_t*>(
            reinterpret_cast<uintptr_t>(fdt.base_ptr)
        )
    );
    
    console.writeln("This should work after relinking");

    MK::KernelSystem sys = {
        .console = relinked_console,
        .regions = relinked_regions,
        .page_allocator = relinked_page_allocator,
        .page_manager = relinked_page_mananger,
        .fdt = relinked_fdt
    };

    MK::KernelSystem* hi_kernel_sys = MK::phy2kvirt_as<MK::KernelSystem*>(reinterpret_cast<uintptr_t>(&sys));
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

    uintptr_t hi_sp;

    /* update stack address to high */
    __asm__ __volatile__(
        "mov %0, sp\n"
        : "=r"(hi_sp)
    );
    hi_sp = MK::phy2kvirt_as<uintptr_t>(hi_sp);
    __asm__ __volatile__(
        "mov sp, %0\n"
        :
        : "r"(hi_sp)
    );

    console.writef("changed stack address to higher address 0x{016h}\n", hi_sp);
    
    /* jump to higher kernel */
   __asm__ __volatile__ (
        "mov x0, %0\n"
        "mov x1, %1\n"
        "mov x2, %2\n"
        "adrp x3, %3\n"
        "add  x3, x3, :lo12:%3\n"
        "br   x3\n"
        :
        : "r"(hi_kernel_sys), "r"(hi_unmap_spans), "r"(unmap_count), "S"(kernel_main)
        : "x0", "x1", "x2", "x3", "memory"
    );
}