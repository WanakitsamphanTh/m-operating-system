#include "kernel_core.hpp"
#include "kernel/el_handler.hpp"
#include "kernel/irq.hpp"
#include "kernel/io.hpp"
#include "mstd/scope_guard.hpp"
#include <cstdint>

using MK::IRQ;

Ctx* common_irq_handler(Ctx* ctx){
    //printk("elrt\t=%u\t", irq.getContext().elr);
    //printk("splrt\t=%u\t", irq.getContext().splr);
    //printk("esr\t=%u\n", irq.getContext().esr);

    auto irq = MK::IRQ::begin();

    MK::KernelConsole console;
    auto code = irq.getCode();
    switch(code){
        case IRQCode::VTimerInterrupt:
            reset_timer();
            break;
        default:
            console.writef("Unknown IRQ {}\n", static_cast<uint64_t>(code));
            break;
    }

    irq.end();
    return ctx;
}