#include "kernel_core.hpp"
#include "kernel/el_handler.hpp"
#include "kernel/irq.hpp"
#include "mstd/scope_guard.hpp"
#include <cstdint>

using MK::IRQ;

void common_irq_handler(Ctx* ctx){
    auto irq = IRQ::getIRQ(ctx);

    printk("elrt\t=%u\t", irq.getContext().elr);
    printk("splrt\t=%u\t", irq.getContext().splr);
    printk("esr\t=%u\n", irq.getContext().esr);

    irq.getCode().then([](IRQCode&& code){
        switch(code){
            case IRQCode::VTimerInterrupt:
                reset_timer();
                printk("Timer interrupt\n");
                break;
            default:
                printk("Unknown IRQ\n");
                break;
        }
    });

}