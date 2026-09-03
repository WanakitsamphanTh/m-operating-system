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
        template<size_t N, class Arg, class... Args>
        constexpr auto writef(comptime_string<N> fmt, Arg&& arg, Args&&... args){
            auto [str, next] = fmt.template split<0,'{'>();
            static_assert(next.len() > 0, "expect '{'");
            auto [spec, remainder] = next.template split<0,'}'>();
            static_assert(spec.len() < next.len(), "expect '}'");
            static_assert(remainder.len() > 0, "number of parameters and format string do not match");

            write(str);

            using T = std::remove_cvref_t<Arg>;
            constexpr auto fmt_spec = default_fmt<T>::spec;
            constexpr auto parsed_fmt = parse_fmt(spec, fmt_spec);

            write_format<buf_t, T>(
                get_buffer(),
                forward<Arg>(arg),
                parsed_fmt
            );
            return writef(remainder, forward<Args>(args)...);
        }

        template<size_t N, class Arg>
        constexpr auto writef(comptime_string<N> fmt, Arg&& arg){
            auto [str, next] = fmt.template split<'{'>();
            static_assert(next.len() > 0, "expect '{'");
            auto [spec, remainder] = next.template split<'}'>();
            static_assert(spec.len() < next.len(), "expect '}'");

            write(str);

            using T = std::remove_cvref_t<Arg>;
            constexpr auto fmt_spec = default_fmt<T>::spec;
            constexpr auto parsed_fmt = parse_fmt(spec, fmt_spec);

            write_format<buf_t, T>(
                get_buffer(),
                forward<Arg>(arg),
                parsed_fmt
            );

            return writef(remainder);
        }

        template<size_t N>
        constexpr auto writef(comptime_string<N> fmt){
            write(fmt);
        }

        template<class Arg, class... Args>
        constexpr auto write(Arg&& arg, Args&&... args) {
            write(forward<Arg>(arg));
            write(forward<Args>(args)...);
        }

        template<class... Args>
        constexpr auto writeln(Args&&... args) {
            write(forward<Args>(args)..., "\n");
        }

        template<size_t N>
        inline constexpr auto write(const char (&str)[N]) {
            get_buffer().write(str, N-1);
        }

        template<size_t N>
        inline constexpr auto write(comptime_string<N> str){
            get_buffer().write(str, N-1);
        }

        template<class Arg>
        constexpr auto write(Arg&& arg) {
            using TConcrete = std::remove_cvref_t<Arg>;
            write_format<buf_t, TConcrete>(get_buffer(), arg, default_fmt<Arg>::spec);
        }

        
    };
}