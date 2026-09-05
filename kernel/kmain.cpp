#include <climits>
#include <cstdint>
#include <cstring>
#include <kernel_core.hpp>
#include <kernel/dtb.hpp>
#include <kernel/io.hpp>
#include "kernel/mem/region.hpp"
#include "mstd/monadic/maybe.hpp"
#include "mstd/scope_guard.hpp"
#include "mstd/string.hpp"

#include <cstddef>
#include <utility>

using mstd::maybe;
using mstd::nothing;
using mstd::some;
using mstd::as_ptr;

MK::Regions regions;

extern "C" int kmain(uint8_t* dtb){
    MK::KernelConsole console;

    /* parse FDT */
    console.writef("FDT addr {}\n", as_ptr(dtb));
    auto fdt = MK::FDT::try_read_fdt(dtb).take("cannot parse FDT");
    console.writeln("already parsed fdt");

    const auto& root = fdt.find_node("").take("cannot find root");
    const auto& mem = fdt.find_node_prefix("memory").take("cannot find the memory node");
    console.writeln("already found root/ and mem/ fdt");

    auto addr_cells = root.find_property("#address-cells", fdt).take("cannot find #address-cells").u32_at(0).take();
    auto size_cells = root.find_property("#size-cells", fdt).take("cannot find #size-cells").u32_at(0).take();
    console.writef("address-cells {} size-cells {}\n", addr_cells, size_cells);

    const auto& mem_reg = mem.find_property("reg", fdt).take("cannot find the property reg");
    auto entry_size = (addr_cells + size_cells) * 4;
    regions.num =  mem_reg.len / entry_size;
    regions.num = regions.num < 8 ? regions.num : 8;

    console.writef("number of banks {}\n", regions.num);

    for(size_t i = 0; i < regions.num; i++){
        size_t off = i * entry_size;
        regions[i].base = 
            uint64_t(mem_reg.u32_at(4 + off).take()) << 32
            | uint64_t(mem_reg.u32_at(off).take());
        regions[i].size = 
                uint64_t(mem_reg.u32_at(8 + off).take()) << 32
                | uint64_t(mem_reg.u32_at(12 + off).take());
        console.writef("regions[{}] base {} size {}\n", i, as_ptr(regions[i].base), regions[i].size);
    }



    return 0;
}