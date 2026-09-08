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

/*
    bootstrap sequence
    - initialize stack and return to EL1 (OK)
    - mask interrupt (OK)       |   void mask_interrupt(uint64_t mask)
    - parse device tree blob and find UART, Memory, GICD
    - initialize uart (OK)      |   void init_uart(uintptr_t uart_base)
        > uart_puts, uart_putc, printk and console work
    - initialize exception vector table (OK)
    - initialize gicd (OK)      |   void init_gicd(uintptr_t gicd_base)
    - initialize timer (OK)     |   void init_timer()
    - unmask interrupts
        > timer works       |   void unmask_interrupt(uint64_t mask)
    - inspect RAM regions
    - initialize physical page allocator
    - initialize page table
    - activate MMU
    - initialize heap
    - enter kernel main (?)

    remaining:
    - initialize file system
    - scheduler
*/

extern "C" int kmain(uint8_t* dtb){
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
    }, [](){
        console.writeln("Cant find property reg in uart");
        kernel_panic();
    });

    console.writeln("initialized UART");

    // init gicd and enable exceptions
    init_exception_vector();

    console.writef("gicd node name: {}\n", gicd.name);

    gicd.find_property("reg", fdt).then_or_else([&](const MK::FDTProperty& prop){
        ::gicd_base = uintptr_t(prop.u32_at(0).take()) << 32 | uintptr_t(prop.u32_at(4).take());
        ::gicd_size = uint64_t(prop.u32_at(8).take()) << 32 | uint64_t(prop.u32_at(12).take());
        console.writef("gicd base : 0x{016h}\n", ::gicd_base);
    }, [](){
        console.writeln("Cant find property reg in gicd");
        kernel_panic();
    });

    init_gicd();
    printk("init gicd\n");
    unmask_interrupt();
    printk("unmasked interrupt\n");
    enable_fp_neon();
    printk("init fpneon\n");

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
            uint64_t(mem_reg.u32_at(12 + off).take()) << 32
            | uint64_t(mem_reg.u32_at(8 + off).take());
        console.writef("regions[{}] base 0x{016h} size 0x{016h}\n", i, regions[i].span.base, regions[i].span.size);
    }    

    page_allocator.init(
        regions,
        MK::PhySpan{_kernel_start_addr, _kernel_size},
        MK::PhySpan{reinterpret_cast<uintptr_t>(fdt.base_ptr), fdt_size}
    );

    //page_manager.map(_kernel_start_addr, _kernel_rodata_end_addr - _kernel_start_addr, MK::MapMode::Id, MK::PageInfo::MemType::Normal, MK::PageInfo::AP::PRO);
    //page_manager.map(_kernel_data_start_addr, _kernel_data_end_addr - _kernel_data_start_addr, MK::MapMode::Id, MK::PageInfo::MemType::Normal, MK::PageInfo::AP::PRW);
    //page_manager.map(dtb, fdt_size, MK::MapMode::Id, MK::PageInfo::MemType::Normal, MK::PageInfo::AP::PRO);
    //page_manager.map(UartBase, 0x10000ull, MK::MapMode::Id, MK::PageInfo::MemType::Device, MK::PageInfo::AP::PRW);
    //page_manager.map(GICDBase, 0x1000ull, MK::MapMode::Id, MK::PageInfo::MemType::Device, MK::PageInfo::AP::PRW);

    //MK::set_page_table(l0_page_table);

    return 0;
}