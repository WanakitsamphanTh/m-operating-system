#pragma once
#include "buffer.hpp"
#include "mstd/fmt/fmt_core.hpp"
#include "mstd/fmt/buffer.hpp"
#include <cstdint>
#include <type_traits>

namespace mstd {
    template<typename T>
    concept signed_integer 
        = std::is_integral_v<T> && std::is_signed_v<T>;

    template<typename T>
    concept unsigned_integer 
        = (std::is_integral_v<T> && !std::is_signed_v<T>) || std::is_pointer_v<T>;

    template<fmt_buffer FmtBuf, signed_integer I>
    fmt_result write_format(FmtBuf& buf, const I& original_val, fmt_spec& fmt){
        int64_t val = static_cast<int64_t>(original_val);
        backward_buffer tmp;
        fmt_result result;
        bool neg = val < 0;
        if(neg) val = -val;

        /* into integer */
        if(val == 0){
            tmp.putc('0');
        } else {
            while(val > 0) {
                char dig = val % 10;
                val /= 10;
                tmp.putc(dig + '0');
            }
        }

        /* zero pad (only when width > tmp.len())*/
        if(fmt.zero_pad && fmt.width > tmp.len()) {
            size_t pad_size = fmt.width - tmp.len();
            while(pad_size-- > 1) tmp.putc('0');
        }

        /* add sign */
        /* neg : add -
            !neg && show_sign : add +
            !show_sign && zero_pad &&  && fmt.width > tmp.len() : add 0
            _ : skip */
        if(neg) tmp.putc('-');
        else if(fmt.show_sign) tmp.putc('+');
        else if(fmt.zero_pad && fmt.width > tmp.len()) tmp.putc('0');

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

    template<fmt_buffer FmtBuf, unsigned_integer UI>
    fmt_result write_format(FmtBuf& buf, const UI& original_val, fmt_spec& fmt){
        uint64_t val = static_cast<uint64_t>(original_val);
        backward_buffer tmp;
        fmt_result result;

        uint64_t base = 10;
        switch(fmt.base){
            case fmt_spec::number_base::bin : base = 2; break;
            case fmt_spec::number_base::oct : base = 8; break;
            case fmt_spec::number_base::hex : base = 16; break;
        }

        /* into integer */
        if(val == 0) tmp.putc('0');
        else while(val){
            char dig = val % base;
            val /= base;
            tmp.putc(dig + '0');
        }

        /* zero pad if zero_pad && width > tmp.len() (leave 2 slots) */
        if(fmt.zero_pad && fmt.width >= tmp.len() + 2){
            auto pad = fmt.width - tmp.len() - 2;
            while(pad--) tmp.putc('0');
        }

        /* show_base && not decimal -> print 0x, 0b, or 0c 
            !show_base && zero_pad -> print 00 if tmp.width > tmp.len()
        */
        if(fmt.show_base && fmt.base != fmt_spec::number_base::dec) {
            switch(fmt.base){
                case fmt_spec::number_base::bin : tmp.putc('b'); break;
                case fmt_spec::number_base::oct : tmp.putc('o'); break;
                case fmt_spec::number_base::hex : tmp.putc('x'); break;
            }
            tmp.putc('0');
        }
        else if(fmt.zero_pad && fmt.width > tmp.len()){
            tmp.putc('0');
            tmp.putc('0');
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
}