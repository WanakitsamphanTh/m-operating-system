#include "mstd/core.hpp"
#include "mstd/fmt.hpp"
#include "kernel_core.hpp"

namespace mstd {
    extern "C" [[noreturn]] void panic(const char* msg) {
        uart_puts(msg);
        uart_putc('\n');
        kernel_panic();
    }

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

};