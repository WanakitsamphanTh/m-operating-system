.include "./arch/uart.inc"
.include "./arch/varg.inc"

.global uart_putc
uart_putc:
1:
    ldr x1, =UART_BASE
    ldr w2, [x1, #UART_FR]
    and w2, w2, #TXFF
    cbnz w2, 1b
    str w0, [x1, #UART_DR]
    ret

.global uart_puts
uart_puts:
    stp x19, x30, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    mov x19, x0
    mov x20, x0
1:
    ldrb w0, [x19]
    cbz w0, 2f
    bl uart_putc
    add x19, x19, 1
    b 1b
2:
    sub x0, x19, x20
    ldp x20, x21, [sp], #16
    ldp x19, x30, [sp], #16
    ret