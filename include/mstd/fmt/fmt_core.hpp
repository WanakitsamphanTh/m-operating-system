#pragma once
#include <cstdint>
#include <cstddef>
#include <concepts>
#include <type_traits>
#include "mstd/monadic/maybe.hpp"
#include "mstd/comptime/string.hpp"

namespace mstd {
    using std::same_as;

    template<class T>
    concept fmt_buffer = requires (T& t, char c, const char* str, size_t len){
        { t.putc(c) } -> same_as<bool>;
        { t.write(str, len) } -> same_as<size_t>;
    };

    class dyn_fmt_buffer {
    public:
        virtual bool putc(char) = 0;
        virtual size_t write(const char*, size_t) = 0;
    };

    struct fmt_result {
        size_t written;
        size_t remainder;
        fmt_result(): written(0), remainder(0){}
    };

    struct fmt_spec {
        enum class alignment {unaligned, left, right} align: 2;
        bool show_sign: 1;
        bool show_base: 1;
        bool zero_pad: 1;
        enum class number_base {bin, oct, dec, hex} base: 3;
        size_t precision;
        size_t width;
    };

    template<fmt_buffer FmtBuf, class T>
    fmt_result write_format(FmtBuf& buf, const T& val, fmt_spec fmt);

    /* 
        default formatter
        - only accepts concrete types as template parameter
        - reference types are decayed
    */

    template<class T>
    struct default_fmt;

    template<class T>
        requires (!std::is_reference_v<T> && !std::is_pointer_v<T>)
    struct default_fmt<T> { 
        static constexpr fmt_spec spec = {
            .align = fmt_spec::alignment::left,
            .show_sign = false,
            .show_base = false,
            .zero_pad = false,
            .base = fmt_spec::number_base::dec,
            .precision = static_cast<size_t>(UINT64_MAX),
            .width = 0  /* sentinel value (undefined width)*/
        }; 
    };

    template<class TRef>
        requires (std::is_reference_v<TRef>)
    struct default_fmt<TRef> {
        using TConcrete = std::remove_cvref_t<TRef>;
        static constexpr fmt_spec spec = default_fmt<TConcrete>::spec;
    };

    void parse_fmt(const char* spec, fmt_spec& fmt){
        /*
        sign: '+' | none
        zeropad: '0' | none
        width: [1..9] digit+ | none
        precision : '.' digit+ | none
        base: b | o | h | none
        alignment: 'L' | 'R' | non
        */
        size_t i = 0;
        if(spec[i] == '+') {
            fmt.show_sign = true;
            i++;
        }
        if(spec[i] == '0') {
            fmt.zero_pad = true;
            i++;
        }
        size_t w = 0;
        while(spec[i] >= '0' && spec[i] <= '9') {
            w = w * 10 + (spec[i] - '0');
            i++;
        }
        if(w != 0) fmt.width = w;
        if(spec[i] == '.'){
            i++;
            size_t p = 0;
            while(spec[i] >= '0' && spec[i] <= '9') {
                p = p * 10 + (spec[i] - '0');
                i++;
            }
            fmt.precision = p;
        }
        switch(spec[i]){
            case 'b':
                fmt.base = fmt_spec::number_base::bin; 
                i++;
                break;
            case 'o': 
                fmt.base = fmt_spec::number_base::oct; 
                i++;
                break;
            case 'h': 
                fmt.base = fmt_spec::number_base::hex; 
                i++;
                break;
        }

        if(spec[i] == 'L'){
            fmt.align = fmt_spec::alignment::left;
            i++;
        } else if(spec[i] == 'R'){
            fmt.align = fmt_spec::alignment::right;
            i++;
        }
    }
}