#pragma once

#include "error.hpp"
#include "fmt_core.hpp"
#include <type_traits>
#include <utility>
#include "integer.hpp"
#include "mstd/comptime/string.hpp"

namespace mstd {
    using std::forward;

    template<class concrete_writer, class buf_t>
    class writer_core {
    protected:
        auto& get_buffer() { return static_cast<concrete_writer*>(this)->buffer(); }
    public:
        template<class Arg, class... Args>
        constexpr auto write(Arg&& arg, Args&&... args) {
            write(arg);
            write(forward<Args>(args)...);
        }

        template<size_t N>
        constexpr auto write(const char (&str)[N]) {
            get_buffer().write(str, N-1);
        }

        template<size_t N>
        constexpr auto write(comptime_string<N> str){
            get_buffer().write(str, N-1);
        }

        template<class Arg>
        constexpr auto write(Arg&& arg) {
            fmt_spec spec = default_fmt<Arg>::spec;
            write_format<buf_t, Arg>(get_buffer(), forward<Arg>(arg), spec);
        }
    };
}