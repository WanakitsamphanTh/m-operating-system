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
    scope_guard guard([]{
        printk("Exit kernel...\n");
    });
    printk("Hello world\n");
    for(volatile int i = 0; i < INT_MAX; i++){
        printk("[kmain] %d\n", i);
    }
    return 0;
}