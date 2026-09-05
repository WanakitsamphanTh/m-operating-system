#include <climits>
#include <cstdint>
#include <cstring>
#include <kernel_core.hpp>
#include <kernel/dtb.hpp>
#include <kernel/io.hpp>
#include "mstd/monadic/maybe.hpp"
#include "mstd/scope_guard.hpp"
#include "mstd/string.hpp"
#include <cstddef>
#include <utility>

using mstd::maybe;
using mstd::nothing;
using mstd::some;
using mstd::scope_guard;

template<typename Ptr>
void* as_ptr(Ptr ptr){ return reinterpret_cast<void*>(ptr); }


extern "C" int kmain(uint8_t* dtb){
    MK::KernelConsole console;

    /* parse FDT */
    console.writef("FDT addr {}\n", as_ptr(dtb));
    auto fdt = MK::FDT::try_read_fdt(dtb).take("cannot parse FDT");
    const auto& mem = fdt.find_node_prefix("memory").take("cannot find the memory node");
    const auto& mem_reg = mem.search_property("reg", fdt).take("cannot find the property reg\n");

    uintptr_t addr = (uint64_t(mem_reg.u32_at(4).take()) << 32) | uint64_t(mem_reg.u32_at(0).take());
    auto size = (uint64_t(mem_reg.u32_at(12).take()) << 32) | uint64_t(mem_reg.u32_at(8).take());
    console.writef("{} addr {016} 0x{016h}\n", mem_reg.get_name(fdt), as_ptr(addr), size);

    return 0;
}