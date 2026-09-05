#include "kernel/uart.hpp"
#include <cstdint>

extern "C" {
    extern const uint64_t _kernel_start_addr;
    extern const uint64_t _kernel_end_addr;
    extern const uint64_t _kernel_size;
    extern const uint64_t _kernel_text_start_addr;
    extern const uint64_t _kernel_text_end_addr;
    extern const uint64_t _kernel_text_size;
    extern const uint64_t _kernel_rodata_start_addr;
    extern const uint64_t _kernel_rodata_end_addr;
    extern const uint64_t _kernel_rodata_size;
    extern const uint64_t _kernel_data_start_addr;
    extern const uint64_t _kernel_data_end_addr;
    extern const uint64_t _kernel_data_size;
    
    extern uint8_t* _el0_page_table;
    extern uint8_t* _el1_page_table;
    extern uint8_t* _el2_page_table;
    extern uint8_t* _el3_page_table;

    extern uint64_t dtb_boot;

    [[noreturn]] void kernel_halt();
    [[noreturn]] void kernel_panic();

    __attribute__((section(".text.boot"))) void disable_timer();

    extern "C" void kprintf(const char* fmt, ...);
}