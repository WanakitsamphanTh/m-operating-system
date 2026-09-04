#include "kernel_core.hpp"
#include <cstdarg>

extern "C" [[noreturn]] void kernel_panic(){
    printk("kernel panic!");
    disable_timer();
    kernel_halt();   
}

/*

enum class printf_type {
    d, u, h, o, s, c, p
};

enum class printf_len {
    l, ll,
}

const char* printf_parse(const char*, mstd::fmt_spec& spec, printf_type& type, printf_len& len);

extern "C" void kprintf(const char* fmt, ...){
    va_list vargs;
    va_start(vargs, fmt);
    const char* iter = fmt;

    MK::UARTBuffer uart;

    while(*iter){
        if(*iter == '%'){
            if(*(iter+1) == '%') {
                uart.putc('%');
                iter += 2;
            }
        } else {
            uart.putc(*(iter++));
        }
    }
    va_end(vargs);
}

const char* printf_parse(const char* fmt, mstd::fmt_spec& spec, printf_type& type, printf_len& len){
    if(*fmt == '+'){
        spec.show_sign = true;
        fmt++;
    }
    if(*fmt == '-'){
        spec.align = mstd::fmt_spec::alignment::right;
        fmt++;
    }

    if(*fmt == '-'){
        spec.zero_pad = true;
        fmt++;
    }
    size_t w = 0;
    while(*fmt >= '0' && *fmt <= '9') {
        w = w * 10 + (*fmt - '0');
        fmt++;
    }
    if(w != 0) spec.width = w;

    if(*fmt == '.'){
        fmt++;
        size_t p = 0;
        while(*fmt >= '0' && *fmt <= '9') {
            p = p * 10 + (*fmt - '0');
            fmt++;
        }
        spec.precision = p;
    }

    switch(*fmt) {

    }

    switch(*fmt) {

    }
}*/