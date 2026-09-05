#pragma once
#include "fmt_core.hpp"
#include "mstd/fmt/fmt_core.hpp"
#include <cstdint>
#include <type_traits>

extern "C" int printk(const char* str, ...);


namespace mstd {
    
    template<fmt_buffer FmtBuf>
    fmt_result write_format_str(FmtBuf& buf, const char* str, fmt_spec fmt){
        fmt_result res{};
        /* later implement format specificaiton*/
        if(str == nullptr)
            buf.write("(null)", 6);
        size_t precision = fmt.precision;
        while(*str && precision){
            buf.putc(*(str++));
            precision--;
        }
        return res;
    }
}