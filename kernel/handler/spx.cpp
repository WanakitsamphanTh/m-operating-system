#include "kernel/el_handler.hpp"
#include "kernel_core.hpp"
#include "kernel/io.hpp"

extern "C" Ctx* sync_spx(Ctx* ctx){
    //printk("from %s\n", __PRETTY_FUNCTION__);
    MK::KernelConsole console;
    console.writef("elr\t= 0x{016h} ", ctx->elr);
    console.writef("splr\t= 0x{016h} ", ctx->splr);
    console.writef("esr\t= 0x{016h}\n", ctx->esr);
    switch(ctx->esr){
        default:
            printk("Unknown synchronous exception\n");
            kernel_panic();
    }
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