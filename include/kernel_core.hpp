#include "kernel/mem/page.hpp"
#include "kernel/uart.hpp"
#include <cstdint>

namespace MK {
    struct PhySpan;
}

extern "C" {
    constexpr uint64_t UartBase = 0x09000000;
    constexpr uint64_t GICDBase = 0x08000000;

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
    
    extern MK::PageDescriptor* _l0_page_table;
    extern MK::PageDescriptor* _l1_page_table;
    extern MK::PageDescriptor* _l2_kernel_page_table;
    extern MK::PageDescriptor* _l2_kernel_device_table;
    extern MK::PageDescriptor* _l3_kernel_page_table;
    extern MK::PageDescriptor* _l3_device_uart_table;
    extern MK::PageDescriptor* _l3_device_gicd_table;

    extern uintptr_t uart_base;
    extern uint64_t uart_size;
    extern uintptr_t gicd_base;
    extern uint64_t gicd_size;
    extern uintptr_t gicr_base;
    extern uint64_t gicr_size;

    extern uint64_t dtb_boot;

    void init_gicd();
    void init_timer();
    void init_exception_vector();
    void enable_fp_neon();
    void unmask_interrupt();

    void mask_interrupt();

    [[noreturn]] void kernel_main();
    
    [[noreturn]] void kernel_halt();
    [[noreturn]] void kernel_panic();

    __attribute__((section(".text.boot"))) void disable_timer();

    void kprintf(const char* fmt, ...);

    void init_global_heap();
}