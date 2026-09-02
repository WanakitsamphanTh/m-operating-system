#include "kernel/el_handler.hpp"
#include "kernel_core.hpp"

extern "C" Ctx* sync_spx(Ctx* ctx){
    //printk("from %s\n", __PRETTY_FUNCTION__);
    common_irq_handler(ctx);
    return ctx;
}

extern "C" Ctx* irq_spx(Ctx* ctx){
    //printk("from %s\n", __PRETTY_FUNCTION__);
    common_irq_handler(ctx);
    return ctx;
}

extern "C" Ctx* fiq_spx(Ctx* ctx){
    printk("from %s\n", __PRETTY_FUNCTION__);
    common_irq_handler(ctx);
    return ctx;
}

extern "C" Ctx* serr_spx(Ctx* ctx){
    printk("from %s\n", __PRETTY_FUNCTION__);
    common_irq_handler(ctx);
    return ctx;
}