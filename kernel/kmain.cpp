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
    uart_puts("Hello world #1\n");
    MK::KernelConsole console;
    console.write("Hello world #2\n");
    console.writeln("Hello world #3");
    int x = 100;
    console.writeln("integer ", 0);
    console.writeln("x =  ", x);
    console.writeln("&x = ", &x);
    console.writeln("Hello",'!', " it's me", '\n');
    return 0;
}