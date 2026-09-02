#pragma once
#include "kernel/mkobj.hpp"
#include "kernel/el_handler.hpp"
#include "mstd/monadic/maybe.hpp"

namespace MK {
    using std::forward;
    using std::move;
    using mstd::maybe;

    template<class Fn>
    class Thread : public virtual CoreObj {
        maybe<Fn> task;
        Ctx* ctx;
    public:
        /* constructors */
        Thread(Fn&& fn): task(some<Fn>(forward<Fn>(fn))){}
        Thread(Thread&& other): task(move(other.task)), ctx(other.ctx){
            other.task = mstd::nothing;
            other.ctx = mstd
        }

        Ctx& getContext() const { return ctx; }
        
        virtual ~Thread() = default;
    }; 
}