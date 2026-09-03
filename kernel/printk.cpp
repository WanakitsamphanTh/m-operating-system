#include "kernel_core.hpp"
#include <cstdarg>
#include <cstdint>

int printk(const char* str, ...){
    va_list args;
    va_start(args, str);
    int nchars = 0;
    while(*str){
        if(*str == '%'){
            str++;
            switch(*str){
                case 's': {
                    const char* s = va_arg(args, const char*);
                    nchars += uart_puts(s);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    uart_putc(c);
                    nchars++;
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    bool neg = d < 0;
                    char buffer[12]; // Enough to hold -2147483648 and null terminator
                    int ndigits = 0;
                    while(d != 0){
                        int digit = d % 10;
                        buffer[ndigits++] = '0' + (digit < 0 ? -digit : digit);
                        d /= 10;
                    }
                    if(ndigits == 0) {
                        buffer[ndigits++] = '0';
                    }
                    // Reverse the buffer
                    for(int i = 0; i < ndigits / 2; i++){
                        char temp = buffer[i];
                        buffer[i] = buffer[ndigits - i - 1];
                        buffer[ndigits - i - 1] = temp;
                    }
                    buffer[ndigits] = '\0'; // Null terminate the string
                    if(neg){
                        uart_putc('-');
                        nchars++;
                    }
                    nchars += uart_puts(buffer);
                    break;
                }
                case 'u': {
                    unsigned long long int d = va_arg(args, unsigned long long int);
                    char buffer[12]; // Enough to hold -2147483648 and null terminator
                    int ndigits = 0;
                    while(d != 0){
                        int digit = d % 10;
                        buffer[ndigits++] = '0' + (digit < 0 ? -digit : digit);
                        d /= 10;
                    }
                    if(ndigits == 0) {
                        buffer[ndigits++] = '0';
                    }
                    // Reverse the buffer
                    for(int i = 0; i < ndigits / 2; i++){
                        char temp = buffer[i];
                        buffer[i] = buffer[ndigits - i - 1];
                        buffer[ndigits - i - 1] = temp;
                    }
                    buffer[ndigits] = '\0'; // Null terminate the string
                    nchars += uart_puts(buffer);
                    break;
                }
                case '%': {
                    uart_putc('%');
                    nchars++;
                    break;
                }
                case 'p': {
                    void* p = va_arg(args, void*);
                    char buffer[19]; // Enough to hold "0x" + 16 hex digits + null terminator
                    buffer[0] = '0';
                    buffer[1] = 'x';
                    for(int i = 0; i < 16; i++){
                        int nibble = ((uintptr_t)p >> (60 - i * 4)) & 0xF;
                        buffer[2 + i] = (nibble < 10) ? ('0' + nibble) : ('a' + nibble - 10);
                    }
                    buffer[18] = '\0'; // Null terminate the string
                    nchars += uart_puts(buffer);
                    break;
                }
                default:
                    uart_putc('%');
                    uart_putc(*str);
                    nchars += 2;
            }
        }
        else {
            uart_putc(*str);
        }
        str++;
        nchars++;
    }
    va_end(args);
    return nchars;
}