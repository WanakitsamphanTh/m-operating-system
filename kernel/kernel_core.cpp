#include "kernel_core.hpp"
#include "kernel/mem/page.hpp"
#include <cstdarg>
#include <cstdint>

MK::PageDescriptor* _l0_page_table;
MK::PageDescriptor* _l1_page_table;
MK::PageDescriptor* _l2_kernel_page_table;
MK::PageDescriptor* _l2_kernel_device_table;
MK::PageDescriptor* _l3_kernel_page_table;
MK::PageDescriptor* _l3_device_uart_table;
MK::PageDescriptor* _l3_device_gicd_table;

extern "C" [[noreturn]] void kernel_panic(){
    printk("kernel panic!");
    //mask_interrupt();
    disable_timer();
    kernel_halt();   
}
