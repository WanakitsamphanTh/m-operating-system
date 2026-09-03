#include "kernel/uart.hpp"
#include "mstd/fmt.hpp"
#include <type_traits>

extern "C" int printk(const char* str, ...);

namespace MK {
    using std::forward;
    
    template<class... Args>
    mstd::fmt_result kprintf(const char* fmt, Args...);
    mstd::fmt_result kprintf(const char* str);

    class UARTBuffer {
        public:
            bool putc(char c){
                uart_putc(c);
                return true;
            }
            size_t write(const char* str, size_t n){
                auto c = n;
                while(n--) uart_putc(*(str++));
                return c;
            }
        };
    class KernelConsole: public mstd::writer_core<KernelConsole, UARTBuffer> {
        UARTBuffer uart_buf;
    public:
        UARTBuffer& buffer() { return uart_buf; }
    };
}