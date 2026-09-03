#pragma once

#include "error.hpp"
#include "fmt_core.hpp"
#include <type_traits>
#include <utility>
#include <string_view>

namespace mstd {
    using std::forward;
    using std::string_view;

    template<class ConcreteWriter>
    class writer_core {
    protected:
        auto getBuffer() { return static_cast<ConcreteWriter*>(this)->buffer(); }
        template<size_t N, class... Args>
        void write_literal(string_view fmt) {
            getBuffer()->write(fmt, fmt.size());
        }
    public:
        template<class Arg, class... Args>
        result<void, format_error> write(string_view fmt, Arg&& arg, Args&&... args){
            consteval size_t beg = 0;
            consteval size_t end = beg;
            while(end < fmt.size() && fmt[end] != '{') 
                end++;
            write_literal(fmt.substr(beg, end));
            
            /* found {*/
            if constexpr(end < fmt.size()){
                beg = end;
                while(end < fmt.size() && fmt[end] != '}')
                    end++;

                /* closing } not found */
                if(end == fmt.size())
                    return some<format_error>(format_error::invalid_specifier);
                
                using ConcreteArg = remove_cv_t<std::remove_reference_t<Arg>>;
                using BufferType = std::remove_reference_t<decltype(getBuffer())>;
                fmt_spec spec = default_fmt<ConcreteArg>::spec;
                write_format<BufferType, ConcreteArg>(getBuffer(), arg, spec);
                return write(fmt.substring(end, fmt.size()), forward<Args>(args)...);
            } else {
                return Ok;
            }
        }

        result<void, format_error> write(string_view fmt) {
            write_literal(fmt);
            return Ok;
        }

        template<size_t N, class... Args>
        result<void, format_error> writeln(string_view fmt, Args... args){
            auto write_result = write(fmt, forward<Args>(args)...);
            if(write_result.is_ok()) getBuffer()->putc('\n');
            return write_result;
        }

        result<void, format_error> write_ln(string_view fmt) {
            write_literal(fmt);
            getBuffer()->putc('\n');
            return Ok;
        }
    };
}