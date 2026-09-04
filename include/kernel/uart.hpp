#pragma once
extern "C" int uart_puts(const char*);
extern "C" void uart_putc(char);
extern "C" int printk(const char* str, ...);
