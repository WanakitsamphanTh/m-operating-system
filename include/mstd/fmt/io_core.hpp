#pragma once

#include "string.h"
#include "error.hpp"
#include "fmt_core.hpp"
#include <cstdint>
#include <type_traits>
#include <utility>

namespace mstd {
    using std::forward;
    using std::remove_cvref_t;
    using std::decay_t;
    using std::add_const_t;
    using std::remove_pointer_t;

    template<class buf_t>
    using fmt_fn = void (*)(buf_t&, const void*, fmt_spec&);

    /*
    template<class buf_t, class T>
    struct writef_implementation {
        static void fn(buf_t& w, const void* ptr, fmt_spec& spec){
            const T& arg = *reinterpret_cast<const T*>(ptr);
            write_format<buf_t>(w, arg, spec);
        }
    };*/

    template<class buf_t, class T>
    struct writef_implementation {
        static void fn(buf_t& w, const void* ptr, fmt_spec& spec){
            if constexpr (std::is_array_v<T>){
                using Te =  remove_pointer_t<decay_t<T>>;
                const Te* arg = reinterpret_cast<const Te*>(ptr);
                write_format<buf_t>(w, arg, spec);
            } else {
                const T& arg = *reinterpret_cast<const T*>(ptr);
                write_format<buf_t>(w, arg, spec);
            }
        }
    };

    template<class buf_t>
    struct erased_arg {
        fmt_fn<buf_t> fn;
        const void* arg_ptr;
        fmt_spec spec;
    };

    template<class concrete_writer, class buf_t>
    class writer_core {
    protected:
        auto& get_buffer() { return static_cast<concrete_writer*>(this)->buffer(); }
        void writef_dispatch(const char *fmt, erased_arg<buf_t>* args, size_t arg_len){
            auto iter = fmt;
            size_t i = 0;
            const char* fmt_beg;
            const char* fmt_end;
            while(*iter){
                if(*iter == '{'){
                    fmt_beg = iter + 1;
                    if(*fmt_beg == '\0') {
                        /* some form of error handling*/
                        break;
                    }
                    fmt_end = fmt_beg;
                    while(*fmt_end && *fmt_end != '}') 
                        fmt_end++;
                    
                    if(*fmt_end == '\0') {
                        /* some form of error handling*/
                        break;
                    }

                    if(i < arg_len) {
                        parse_fmt(fmt_beg, args[i].spec);
                        args[i].fn(get_buffer(), args[i].arg_ptr, args[i].spec);
                        i++;
                    } else {
                        /* some form of error handling*/
                    }
                    iter = fmt_end + 1;
                } else if(*iter == '\'' && *(iter + 1) == '{'){
                    get_buffer().putc(*iter);
                    iter += 2;
                } else {
                    get_buffer().putc(*(iter++));
                }
            }
        }
    public:

        template<class... Args>
        inline void writef(const char* fmt, const Args&... args) {
            if constexpr(sizeof...(Args) == 0){
                writef_dispatch(fmt, strlen(fmt), nullptr, 0);
            } else {
                erased_arg<buf_t> arg_list[] = {
                    erased_arg<buf_t>{
                        .fn = &writef_implementation<
                            buf_t,
                            remove_cvref_t<Args>
                        >::fn,
                        .arg_ptr = &args,
                        .spec = default_fmt<Args>::spec
                    }...
                };
                writef_dispatch(fmt, arg_list, sizeof...(Args));
            }
        }

        template<class... Args>
        inline void write(Args&&... args) {
            (write(forward<Args>(args)), ...);
        }

        template<class... Args>
        auto writeln(Args&&... args) {
            (write(forward<Args>(args)), ...);
            write('\n');
        }

        template<size_t N>
        inline auto write(const char (&str)[N]) {
            get_buffer().write(str, N-1);
        }

        template<size_t N>
        inline auto write(const char* str) {
            while(*str) get_buffer().putc(*str);
        }

        template<class Arg>
        inline auto write(Arg&& arg) {
            using TConcrete = std::remove_cvref_t<Arg>;
            write_format<buf_t>(get_buffer(), forward<Arg>(arg), default_fmt<Arg>::spec);
        }
    };
}

