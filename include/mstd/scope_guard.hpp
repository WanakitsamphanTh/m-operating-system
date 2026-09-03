#pragma once
#include <utility>

namespace mstd {
    template<class Fn>
    concept deferrable = requires(Fn fn) { { fn() } ; };

    template<deferrable Fn, bool dismissible = false>
    class scope_guard {
        Fn fn;
    public:
        constexpr scope_guard(Fn&& fn): fn(std::forward<Fn>(fn)){}
        ~scope_guard() { fn(); }
    };

    template<deferrable Fn>
    class scope_guard<Fn, true> {
        Fn fn;
        bool active;
    public:
        constexpr scope_guard(Fn&& fn): fn(std::forward<Fn>(fn)), active(true){}
        constexpr void dismiss() { active = false; }
        ~scope_guard() { if(active) fn(); }
    };

    template<deferrable Fn>
    using dismissible_guard = scope_guard<Fn, true>;
}