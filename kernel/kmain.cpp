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
MK::Allocator allocator;

extern "C" int kmain(uint8_t* dtb){
    /* parse FDT */
    console.writef("FDT addr {}\n", as_ptr(dtb));
    auto fdt = MK::FDT::try_read_fdt(dtb).take("cannot parse FDT");
    console.writeln("already parsed fdt");

    const auto& root = fdt.find_node("").take("cannot find root");
    const auto& mem = root.find_node_prefix("memory").take("cannot find the memory node");

    fdt.print(console);

    auto addr_cells = root.find_property("#address-cells", fdt).take("cannot find #address-cells").u32_at(0).take();
    auto size_cells = root.find_property("#size-cells", fdt).take("cannot find #size-cells").u32_at(0).take();
    console.writef("address-cells {} size-cells {}\n", addr_cells, size_cells);

    console.writeln("==============================================");
    console.writef("device tree starts at\t{} ends at\t{}\n", as_ptr(dtb), as_ptr(dtb + 4 * 9 + fdt.struct_size + fdt.string_size));
    console.writef("kernel starts at\t{} ends at\t{}\n", as_ptr(_kernel_start_addr), as_ptr(_kernel_end_addr));
    console.writeln("==============================================");

    const auto& mem_reg = mem.find_property("reg", fdt).take("cannot find the property reg");
    auto entry_size = (addr_cells + size_cells) * 4;
    regions.num =  mem_reg.len / entry_size;
    regions.num = regions.num < 8 ? regions.num : 8;

    console.writef("number of banks {}\n", regions.num);
    
    for(size_t i = 0; i < regions.num; i++){
        size_t off = i * entry_size;
        regions[i].base = 
            uint64_t(mem_reg.u32_at(off).take()) << 32
            | uint64_t(mem_reg.u32_at(off + 4).take());
        regions[i].size = 
            uint64_t(mem_reg.u32_at(12 + off).take()) << 32
            | uint64_t(mem_reg.u32_at(8 + off).take());
        console.writef("regions[{}] base 0x{016h} size 0x{016h}\n", i, regions[i].base, regions[i].size);
    }

    allocator.init(
        _kernel_start_addr,
        _kernel_size,
        regions
    );

    /* set up page table
    // 4 KiB granule
    _l0_page_table = reinterpret_cast<MK::PageDescriptor*>(page_start);
    mstd::memset(_l0_page_table, 0, 4096);
    _l1_page_table = reinterpret_cast<MK::PageDescriptor*>(page_start + 4096);
    mstd::memset(_l1_page_table, 0, 4096);
    _l2_page_table = reinterpret_cast<MK::PageDescriptor*>(page_start + 4096*2);
    mstd::memset(_l2_page_table, 0, 4096);
    _l3_kernel_page_table = reinterpret_cast<MK::PageDescriptor*>(page_start + 4096*3);
    mstd::memset(_l3_kernel_page_table, 0, 4096);
    _l3_device_page_table = reinterpret_cast<MK::PageDescriptor*>(page_start + 4096*4);
    mstd::memset(_l3_device_page_table, 0, 4096);

    // set up table descriptor with 0b11 (valid descriptor bits)
    _l0_page_table[0] = MK::PageDescriptor::make_table(_l1_page_table);
    _l1_page_table[0] = MK::PageDescriptor::make_table(_l2_page_table);
    _l2_page_table[0] = MK::PageDescriptor::make_table(_l3_kernel_page_table);
    _l2_page_table[1] = MK::PageDescriptor::make_table(_l3_device_page_table);
    */

    /*
    {
        size_t i = 0;
        uint64_t addr;
        for(uint64_t addr = 0x40000000ull; addr < _kernel_rodata_end_addr; addr += 4096ull)
            _l3_kernel_page_table[i++]
                = MK::PageDescriptor::make_block(
                    addr, 
                    MK::PageDescriptor::MemoryType::Normal
                    MK::PageDescriptor::AP::PRO
            );
        addr = align<4096>(addr);
        i = (addr - 0x40000000ull) / 4096ull;
        for(; addr < _kernel_data_end_addr; addr += 4096ull)
            _l3_kernel_page_table[i++]
                = MK::PageDescriptor::make_block(
                    addr, 
                    MK::PageDescriptor::MemoryType::Normal
                    MK::PageDescriptor::AP::PRW
            );
    }


    _l3_device_page_table[UartBase / 0x1000ull] = MK::PageDescriptor::make_block(
            UartBase, 
            MK::PageDescriptor::MemoryType::Device
            MK::PageDescriptor::AP::PRW
        );
    _l3_device_page_table[GICDBase / 0x1000ull] = MK::PageDescriptor::make_block(
            GICDBase, 
            MK::PageDescriptor::MemoryType::Device
            MK::PageDescriptor::AP::PRW
        );
    */

    MK::set_page_table(_l0_page_table);

    return 0;
}