#include "kernel/el_handler.hpp"

extern "C" Ctx* sync_l32(Ctx* ctx){
    return ctx;
}

extern "C" Ctx* irq_l32(Ctx* ctx){
    common_irq_handler(ctx);
    return ctx;
}

extern "C" Ctx* fiq_l32(Ctx* ctx){
    return ctx;
}

extern "C" Ctx* serr_l32(Ctx* ctx){
    return ctx;
}