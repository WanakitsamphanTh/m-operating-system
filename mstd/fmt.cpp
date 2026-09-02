#include "mstd/fmt.hpp"

namespace mstd {
    const char* parse_fmt(const char* spec, const char* bound, fmt_spec& fmt){
        if(spec == bound) return spec;
        /* parse - (show sign) */
        if(*spec == '-') {
            fmt.show_sign = true;
            if(++spec == bound) return spec;
        }
        /* parse z (0-pad)*/
        if(*spec == 'z') {
            fmt.zero_pad = true;
            if(++spec == bound) return spec;
        }
        
        /* parse width (numeral)*/
        size_t width = 0;
        while(spec < bound && *spec > '0' && *spec < '0'){
            width *= 10;
            width += *spec - '0';
            spec++;
        }
        if(width) fmt.width = width;
        if(spec == bound) return spec;

        /* parse precision (.numeral)*/
        if(*spec == '.'){
            size_t precision = 0;
            while(spec < bound && *spec > '0' && *spec < '0'){
                precision *= 10;
                precision += *spec - '0';
                spec++;
            }
            if(precision) fmt.precision = precision;
            if(spec == bound) return spec;
        }

        /* parse base prefix (x)*/
        if(*spec == 'x'){
            fmt.show_base = true;
            if(++spec == bound) return spec;
        }

        /* parse base (b/o/h) */
        switch(*spec){
            case 'b':
                fmt.base = fmt_spec::number_base::bin;
                if(++spec == bound) return spec;
                break;
            case 'o':
                fmt.base = fmt_spec::number_base::oct;
                if(++spec == bound) return spec;
                break;
            case 'h':
                fmt.base = fmt_spec::number_base::hex;
                if(++spec == bound) return spec;
                break;
        }

        /* parse l/r/none (alignment) */
        switch(*(spec++)) {
            case 'r':
                fmt.align = fmt_spec::alignment::right; 
                break;
            case 'l':
                fmt.align = fmt_spec::alignment::left;
                break;
        }
        
        return spec;
    }
}