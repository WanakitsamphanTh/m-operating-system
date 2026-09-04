#include <climits>
#include <kernel_core.hpp>
#include "mstd/monadic/maybe.hpp"
#include "mstd/scope_guard.hpp"
#include <cstddef>
#include <utility>

using mstd::maybe;
using mstd::nothing;
using mstd::some;
using mstd::scope_guard;

extern "C" int kmain(){
    uart_puts("Try write/writeln\n");
    MK::KernelConsole console;
    console.write("Hello world #1\n");
    console.writeln("Hello world #2");
    int x = 100;
    console.writeln("integer ", 0);
    console.writeln("x =  ", x);
    console.writeln("&x = ", &x);
    console.writeln("Hello",'!', " it's me", '\n');

    console.writef("Hello {06} {+06} {+06} {} {06} {06} {}\n", 
                    x, x, -x, &x, 10, -1, 'c');

    console.writef("Hello {} {.3}\n", "World", "World");

    char str[] = {'O','S',' ', 'S', 'e', 'n', 'd', 'a','i'};
    console.writef("Hello {} {}\n", str, static_cast<const char*>(str));
    char* str_ref = str;
    console.writef("Hello {} {}\n", str_ref, static_cast<const char*>(str_ref));
    return 0;
}

