#include "kernel/uart.hpp"
#include <cstdint>

extern "C" {
    extern const uint64_t _kernel_start;
    extern const uint64_t _kernel_end;
    extern const uint64_t _kernel_size;
    extern const uint64_t _rodata_start;
    extern const uint64_t _rodata_end;
    extern const uint64_t _rodata_size;
    [[noreturn]] void kernel_halt();
    [[noreturn]] void kernel_panic();

    __attribute__((section(".text.boot"))) void disable_timer();

    extern "C" void kprintf(const char* fmt, ...);
}