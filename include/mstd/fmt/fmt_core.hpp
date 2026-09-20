#pragma once
#include <cstdint>
#include <cstddef>
#include <concepts>
#include <type_traits>
#include "mstd/monadic/maybe.hpp"

namespace mstd {
    using std::same_as;
    using std::remove_cvref_t;
    using std::is_same_v;

    template<class T>
    concept fmt_buffer = requires (T& t, char c, const char* str, size_t len){
        { t.putc(c) } -> same_as<bool>;
        { t.write(str, len) } -> same_as<size_t>;
    };

    struct fmt_result {
        size_t written;
        size_t remainder;
        fmt_result(): written(0), remainder(0){}
        fmt_result(size_t written, size_t remainder)
        : written(written), remainder(remainder){}
    };

    class dyn_fmt_buffer;

    template<class T>
    concept concrete_buffer 
        = fmt_buffer<T> && !is_same_v<remove_cvref_t<T>, dyn_fmt_buffer>;
    
    class dyn_fmt_buffer{
        using any = void*;

        struct vtable {
            bool (*putc)(any, char);
            fmt_result (*write)(any, const char*, size_t);
        };

        const vtable* vptr;
        mutable any writer;

        template<concrete_buffer Buffer>
        static bool impl_putc(any writer, char c){
            return reinterpret_cast<Buffer*>(writer)->putc(c);
        }
        template<concrete_buffer Buffer>
        static fmt_result impl_write(any writer, const char* str, size_t len){
            return reinterpret_cast<Buffer*>(writer)->write(str, len);
        }

        template<concrete_buffer Buffer>
        const vtable* get_vtable(){
            static const vtable vt {
                .putc = &dyn_fmt_buffer::impl_putc<Buffer>,
                .write = &dyn_fmt_buffer::impl_write<Buffer>
            };
            return &vt;
        }

    public:
        dyn_fmt_buffer();
        dyn_fmt_buffer(const dyn_fmt_buffer& writer);
        
        template<concrete_buffer Buffer>
        dyn_fmt_buffer& operator=(Buffer& writer){
            this->vptr = get_vtable<Buffer>();
            this->writer = &writer;
        }

        template<concrete_buffer Buffer>
        dyn_fmt_buffer(Buffer& writer)
            : vptr(get_vtable<Buffer>()), 
            writer(&writer){}

        dyn_fmt_buffer& operator=(const dyn_fmt_buffer& writer);
        
        /*
        caution: type identity is valid only within the same linkage unit
                and downcasting is usually discouraged.
        */
        template<concrete_buffer Buffer>
        bool is() const { return this->vptr == get_vtable<Buffer>(); }
        template<concrete_buffer Buffer>
        maybe<Buffer&> downcast() { 
            if(is<Buffer>()) return some<Buffer&>(*reinterpret_cast<Buffer*>(writer));
            else return nothing;
        }

        bool putc(char c);
        fmt_result write(const char* str, size_t len);
        bool unchecked_putc(char c);
        fmt_result unchecked_write(const char* str, size_t len);
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

    //template<fmt_buffer FmtBuf, class T>
    //fmt_result write_format(FmtBuf& buf, const T& val, fmt_spec fmt);

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

    inline void parse_fmt(const char* spec, fmt_spec& fmt){
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