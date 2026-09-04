#pragma once
#include "el_handler.hpp"
#include "mstd/monadic/maybe.hpp"
#include "kernel/el_handler.hpp"
#include <cstdint>

extern "C" uint64_t get_irq();
extern "C" uint64_t end_irq(uint64_t irq);

using mstd::maybe;
using mstd::some;
using mstd::nothing;

namespace MK {
    class IRQ {
        uint64_t iar;
    public:
        IRQ(uint64_t iar): iar(iar){}
        static IRQ begin(){
            return IRQ(get_irq());
        }

        void end() const {
            end_irq(iar);
        }
        
        IRQCode getCode() const {
            return IRQCode{iar};
        }
    };
}