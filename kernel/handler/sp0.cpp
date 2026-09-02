#include "kernel/el_handler.hpp"
#include "kernel_core.hpp"


extern "C" Ctx* sync_sp0(Ctx* ctx){
    printk("from %s\n", __PRETTY_FUNCTION__);
    common_irq_handler(ctx);
    return ctx;
}

extern "C" Ctx* irq_sp0(Ctx* ctx){
    printk("from %s\n", __PRETTY_FUNCTION__);
    common_irq_handler(ctx);
    return ctx;
}

extern "C" Ctx* fiq_sp0(Ctx* ctx){
    printk("from %s\n", __PRETTY_FUNCTION__);
    common_irq_handler(ctx);
    return ctx;
}

extern "C" Ctx* serr_sp0(Ctx* ctx){
    printk("from %s\n", __PRETTY_FUNCTION__);
    common_irq_handler(ctx);
    return ctx;
}