#pragma once
#include "mstd/monadic/maybe.hpp"
#include "kernel/el_handler.hpp"

extern "C" uint64_t get_irq();
extern "C" uint64_t end_irq(uint64_t irq);

using mstd::maybe;
using mstd::some;
using mstd::nothing;

namespace MK {
    class IRQ {
        Ctx& ctx;
        maybe<IRQCode> code;
    public:
        static IRQ getIRQ(Ctx* ctx){
            return IRQ(*ctx, get_irq());
        }
        
        IRQ(Ctx& ctx, uint64_t irq): ctx(ctx), code(some<IRQCode>(static_cast<IRQCode>(irq))){}
        IRQ(const IRQ&) = delete;
        IRQ(IRQ&& other): ctx(other.ctx), code(other.code){
            code = nothing;
        }

        Ctx& getContext() {
            return ctx;
        }
        
        maybe<IRQCode> getCode(){
            return code;
        }
        
        ~IRQ(){
            if(code)
                end_irq(static_cast<uint64_t>(*code));
        }
    };
}