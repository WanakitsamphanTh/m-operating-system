#pragma once
#include <cstdint>


#pragma pack(1)
struct Ctx {
    uint64_t regs[31];
    uint64_t elr;
    uint64_t splr;
    uint64_t esr;
};

extern "C" int reset_timer();

extern "C" Ctx* sync_sp0(Ctx* ctx);
extern "C" Ctx* irq_sp0(Ctx* ctx);
extern "C" Ctx* fiq_sp0(Ctx* ctx);
extern "C" Ctx* serr_sp0(Ctx* ctx);


extern "C" Ctx* sync_spx(Ctx* ctx);
extern "C" Ctx* irq_spx(Ctx* ctx);
extern "C" Ctx* fiq_spx(Ctx* ctx);
extern "C" Ctx* serr_spx(Ctx* ctx);

extern "C" Ctx* sync_l64(Ctx* ctx);
extern "C" Ctx* irq_l64(Ctx* ctx);
extern "C" Ctx* fiq_l64(Ctx* ctx);
extern "C" Ctx* serr_l64(Ctx* ctx);

extern "C" Ctx* sync_l32(Ctx* ctx);
extern "C" Ctx* irq_l32(Ctx* ctx);
extern "C" Ctx* fiq_l32(Ctx* ctx);
extern "C" Ctx* serr_l32(Ctx* ctx);

void common_irq_handler(Ctx* ctx);

extern "C" Ctx* return_from_exception(Ctx* ctx);

enum class IRQCode: uint64_t {
    VTimerInterrupt = 27,
    PTimerInterrupt = 30
};