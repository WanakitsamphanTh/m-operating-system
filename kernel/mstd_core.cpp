#include "mstd/core.hpp"
#include "kernel_core.hpp"

namespace mstd {
    extern "C" [[noreturn]] void panic(const char* msg) {
        uart_puts(msg);
        uart_putc('\n');
        kernel_panic();
    }
};