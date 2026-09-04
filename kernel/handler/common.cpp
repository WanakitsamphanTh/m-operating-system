#include "kernel_core.hpp"
#include "kernel/el_handler.hpp"
#include "kernel/irq.hpp"
#include "mstd/scope_guard.hpp"
#include <cstdint>

using MK::IRQ;

Ctx* common_irq_handler(Ctx* ctx){
    //printk("elrt\t=%u\t", irq.getContext().elr);
    //printk("splrt\t=%u\t", irq.getContext().splr);
    //printk("esr\t=%u\n", irq.getContext().esr);

    auto irq = MK::IRQ::begin();

    switch(irq.getCode()){
        case IRQCode::VTimerInterrupt:
            //printk("\ninterrupt!\n");
            reset_timer();
            break;
        default:
            //printk("Unknown IRQ\n");
            break;
    }

    irq.end();
    return ctx;
}