#pragma once
#include <cstdint>
#include "kernel/mem/mem.hpp"

#pragma pack(1)
struct Ctx {
    uint64_t regs[31];
    uint64_t elr;
    uint64_t splr;
    uint64_t esr;
};

namespace MK {
    using Stack = uint8_t[page_size];
    using StackPtr = uint8_t*;

    typedef void (&thread_fn)();

    template<typename Fn>
    class Thread {
        Ctx* ctx;
        StackPtr stack_top;
        Fn fn;
    public:
        Thread(Fn&& fn);
        Thread(Thread& thread);
        ~Thread();
    };
}