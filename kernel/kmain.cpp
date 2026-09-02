#include <climits>
#include <kernel_core.hpp>
#include "mstd/monadic/maybe.hpp"
#include "mstd/scope_guard.hpp"
#include <cstddef>

using mstd::maybe;
using mstd::nothing;
using mstd::some;
using mstd::scope_guard;

extern "C" int kmain(){
    MK::KernelConsole console;
    console.write("Hello world\n");
    console.write("This is {}", 100);
    return 0;
}