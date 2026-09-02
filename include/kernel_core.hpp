#include "kernel/printk.hpp"
#include "kernel/uart.hpp"

extern "C" [[noreturn]] void kernel_halt();
extern "C" [[noreturn]] void kernel_panic();