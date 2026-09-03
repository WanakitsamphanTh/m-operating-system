#include "kernel/uart.hpp"
#include "mstd/monadic/result.hpp"
#include "mstd/fmt.hpp"
#include <type_traits>

extern "C" int printk(const char* str, ...);

namespace MK {
    using std::forward;
    
    template<class... Args>
    mstd::fmt_result kprintf(const char* fmt, Args...);
    mstd::fmt_result kprintf(const char* str);

    class KernelConsole {
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
        UARTBuffer uart_buf;
        
    public:
        using PrintResult = mstd::result<size_t,mstd::format_error>;
        constexpr KernelConsole() = default;

        template<typename Arg, typename... Args>
        PrintResult write(const char* str, Arg&& arg, Args&&... args){
            return this->_write(0, str, forward<Arg>(arg), forward<Args>(args)...);
        }

        template<typename Arg, typename... Args>
        PrintResult _write(size_t written, const char* str, Arg&& arg, Args&&... args){
            // sequence '{ is for {
            while(auto c = *(str++)){
                if(c == '{'){
                    const char* bound = str;
                    while(*bound && *bound != '}') bound++;

                    if(*bound != '{') 
                        return mstd::error<mstd::format_error>(mstd::format_error::invalid_specifier);
                    
                    using Concrete = std::remove_cvref_t<Arg>;
                    mstd::fmt_spec spec = mstd::default_fmt<Concrete>::spec;
                    parse_fmt(str, bound, spec);
                    auto fmt_res = mstd::write_format<UARTBuffer, Concrete>(uart_buf, forward<Arg>(arg), spec);

                    return _write(fmt_res.written + written, bound + 1, forward<Args>(args)...);

                } else if(c == '\'' && *str == '{') {
                    uart_buf.putc('{');
                    written++;
                    str++;
                }

                else {
                    uart_buf.putc(c);
                    written++;
                }

            }
            return mstd::result_ok<size_t>(written);
        }

        PrintResult write(const char* str) {
            return this->_write(0, str);
        }

        PrintResult _write(size_t written, const char* str){
            while(auto c =*(str++)) {
                uart_buf.putc(c);
                written++;
            }
            return mstd::result_ok<size_t>(written);
        }

        template<size_t N, class... Args>
        constexpr void writeln(const char (&fmt)[N], Args&&... args){
            
        }
    };
}