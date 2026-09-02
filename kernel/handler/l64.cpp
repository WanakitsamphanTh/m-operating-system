#include "kernel/el_handler.hpp"

extern "C" Ctx* sync_l64(Ctx* ctx){
    return ctx;
}

extern "C" Ctx* irq_l64(Ctx* ctx){
    common_irq_handler(ctx);
    return ctx;
}

extern "C" Ctx* fiq_l64(Ctx* ctx){
    return ctx;
}

extern "C" Ctx* serr_l64(Ctx* ctx){
    return ctx;
}