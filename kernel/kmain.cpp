#include <climits>
#include <cstdint>
#include <kernel_core.hpp>
#include <kernel/io.hpp>
#include "mstd/monadic/maybe.hpp"
#include "mstd/scope_guard.hpp"
#include <cstddef>
#include <utility>

using mstd::maybe;
using mstd::nothing;
using mstd::some;
using mstd::scope_guard;

char str[] = {'O','S',' ', 'S', 'e', 'n', 'd', 'a','i'};

extern "C" int kmain(uintptr_t dtb){
    uart_puts("Try write/writeln\n");
    MK::KernelConsole console;

    console.writef("[DTB] addr {} ", dtb);
    console.writef("[kernel] start {8h} end {8h} size {}\n", _kernel_start, _kernel_end, _kernel_size);
    console.writef("[rodata] start {8h} end {8h} size {}\n", _rodata_start, _rodata_end, _rodata_size);
    int x = 100;

    console.write("Hello world #1\n");
    console.writeln("Hello world #2");
    console.writeln("integer ", 0);
    console.writeln("x =  ", x);
    console.writeln("&x = ", &x);
    console.writeln("Hello",'!', " it's me", '\n');

    console.writef("Hello {06} {+06} {+06} {} {06} {06} {}\n", 
                    x, x, -x, &x, 10, -1, 'c');

    console.writef("Hello {} {.3}\n", "World", "World");

    console.writef("Hello {} {}\n", str, static_cast<const char*>(str));
    char* str_ref = str;
    console.writef("Hello {} {}\n", str_ref, static_cast<const char*>(str_ref));
    console.writef("true {} false {}", true, false);

    disable_timer();
    return 0;
}

