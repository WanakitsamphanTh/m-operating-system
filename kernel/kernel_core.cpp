#include "kernel_core.hpp"

extern "C" [[noreturn]] void kernel_panic(){
    printk("kernel panic!");
    kernel_halt();   
}