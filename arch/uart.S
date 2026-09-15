.equ UART_BASE, 0x09000000  /* as fallback */
.equ UART_DR,   0x00
.equ UART_FR,   0x18
.equ TXFF,      (1 << 5)

.section .data
    .global uart_base
    .global uart_size
    uart_base: .quad UART_BASE
    uart_size: .quad 0x1000

.text
.global uart_putc
uart_putc:
1:
    adrp x1, uart_base
    add x1, x1, :lo12:uart_base
    ldr x1, [x1]
    ldr w2, [x1, #UART_FR]
    and w2, w2, #TXFF
    cbnz w2, 1b
    str w0, [x1, #UART_DR]
    ret

.global uart_puts
uart_puts:
    mov x9, x0     // x9 := str (iter)
    mov x10, x0     // x10 := str (base)
    adrp x1, uart_base
    addd x1, x1, :lo12:uart_base
    ldr x1, [x1]
1:
    ldrb w0, [x9]
    cbz w0, 1f
2:
    ldr w2, [x1, #UART_FR]
    and w2, w2, #TXFF
    cbnz w2, 2b
    str w0, [x1, #UART_DR]
    add x9, x9, 1
    b 1b
1:
    sub x0, x9, x10
    ret