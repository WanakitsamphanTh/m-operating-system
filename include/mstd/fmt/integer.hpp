#pragma once
#include "buffer.hpp"
#include "mstd/fmt/fmt_core.hpp"
#include "mstd/fmt/buffer.hpp"
#include <cstdint>
#include <type_traits>

namespace mstd {

    template<typename T>
    concept signed_integer 
        = std::is_integral_v<T> && std::is_signed_v<T> && !char_type<T>;

    template<typename T>
    concept unsigned_integer 
        = (std::is_integral_v<T> && !std::is_signed_v<T> && !char_type<T>) || std::is_pointer_v<T>;

    template<typename T>
    concept pointer_type = std::is_pointer_v<T>;

    template<pointer_type TPtr>
    struct default_fmt<TPtr> {
        static constexpr fmt_spec spec = {
            .align = fmt_spec::alignment::left,
            .show_sign = false,
            .show_base = true,
            .zero_pad = true,
            .base = fmt_spec::number_base::hex,
            .precision = static_cast<size_t>(UINT64_MAX),
            .width = 0
        };
    };

    /*  functions implementation */
    template<fmt_buffer FmtBuf>
    fmt_result write_format(FmtBuf& buf, const char& c, fmt_spec fmt){
        fmt_result res{};
        if(buf.putc(c)) res.written++;
        else res.remainder++;
        return res;
    }

    template<fmt_buffer FmtBuf>
    fmt_result write_format_integer_implementation(FmtBuf& buf, uint64_t magnitude, bool neg, fmt_spec fmt){
        static const char digits[] = "0123456789abcdef";
        backward_buffer tmp;
        fmt_result result{};

        uint64_t base = 10;
        switch(fmt.base){
            case fmt_spec::number_base::bin : 
                base = 2; 
                break;
            case fmt_spec::number_base::oct : 
                base = 8; 
                break;
            case fmt_spec::number_base::hex : 
                base = 16; 
                break;
        }

        /* into integer */
        if(magnitude == 0){
            tmp.putc('0');
        } else {
            while(magnitude > 0) {
                char dig = digits[magnitude % 10];
                magnitude /= 10;
                tmp.putc(dig);
            }
        }

        /* zero pad (only when width > tmp.len())*/
        /* leave two slots for either base prefix or sign*/
        if(fmt.zero_pad && fmt.width > tmp.len()) {
            size_t pad_size = fmt.width - tmp.len();
            while(pad_size-- > 2) tmp.putc('0');
        }

        /* other than base 10 : check if type prefix needed*/
        if(base != 10) {
            switch(fmt.base){
                case fmt_spec::number_base::bin : tmp.putc('b'); break;
                case fmt_spec::number_base::oct : tmp.putc('o'); break;
                case fmt_spec::number_base::hex : tmp.putc('x'); break;
            }
            tmp.putc('0');
        } else {
            /* add remaining zero (if needed)*/
            /* add sign
            /* neg : add -
                !neg && show_sign : add +
                !show_sign && zero_pad &&  && fmt.width > tmp.len() : add 0
                _ : skip */
            if(fmt.zero_pad && fmt.width > tmp.len()) tmp.putc('0');
            if(neg) tmp.putc('-');
            else if(fmt.show_sign) tmp.putc('+');
            else if(fmt.zero_pad && fmt.width > tmp.len()) tmp.putc('0');
        }

        /* align if fmt.align != unaligned only when width > tmp.len() */
        if(fmt.align != fmt_spec::alignment::unaligned && fmt.width > tmp.len()){
            size_t space_size = fmt.width - tmp.len();
            if(fmt.align == fmt_spec::alignment::right) {
                while(space_size--) tmp.putc(' ');
            } else {
                while(space_size--) {
                    if(buf.putc(' '))
                        result.written++;
                    else {
                        result.remainder += space_size;
                        break;
                    }
                }
            }
        }

        auto len = tmp.len();
        auto written = buf.write(tmp.rbegin(), len);
        result.written += written;
        result.remainder += len - written;

        return result;
    }

    /* template functions */
    template<fmt_buffer FmtBuf, signed_integer I>
    fmt_result write_format(FmtBuf& buf, const I& original_val, fmt_spec fmt){
        bool neg = original_val < 0;
        uint64_t magnitude = neg 
                            ? uint64_t{-(original_val + 1)} - 1 
                            : uint64_t{original_val};
        return write_format_integer_implementation<FmtBuf>(buf, magnitude, neg, fmt);
    }

    template<fmt_buffer FmtBuf, unsigned_integer UI>
    fmt_result write_format(FmtBuf& buf, const UI& original_val, fmt_spec fmt){
        return write_format_integer_implementation<FmtBuf>(buf, original_val, false, fmt);
    }
}