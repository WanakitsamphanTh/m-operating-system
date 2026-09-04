.global strlen
strlen:
    mov x9, x0
    mov x0, xzr
1:
    ldrb w10, [x9, x0]
    cbz w10, 2f
    add x0, x0, #1
    b 1b
2:
    ret

.global strcmp
strcmp: /* bool strcmp(const char*, const char*) */
    mov x9, x0
    mov x0, xzr
1:
    ldrb w10, [x9], #1
    ldrb w11, [x1], #1
    sub w0, w10, w11 
    cbnz w0, 1f
    cbz w10, 1f
    b 1b
1:
    ret

.global memcmp
memcmp: /* bool strcmp(const char*, const char*, size_t) */
    mov x9, x0
    mov x0, xzr
1:
    cbz x2, 1f
    ldrb w10, [x9], #1
    ldrb w11, [x1], #1
    sub w0, w10, w11 
    cbnz w0, 1f
    sub x2, x2, #1
    b 1b
1:
    ret

.global strcpy
strcpy: /* size_t strcmp(char* dst, const char* src) */
    mov x10, x1
1:
    ldrb w9, [x1], #1
    cbz w9, 1f
    strb w9, [x0], #1
    b 1b
1:
    sub x0, x1, x10
    ret

.global memcpy
memcpy: /* size_t memcpy(char* dst, const char* src, size_t) */
    mov x10, x1
1:
    cbz x3, 1f
    ldrb w9, [x1], #1
    strb w9, [x0], #1
    sub x3, x3, #1
    b 1b
1:
    sub x0, x1, x10
    ret

.global memset
memset: /* size_t memset(char*, char, size_t) */
    mov x10, x0
1:
    cbz x3, 1f
    strb w2, [x0], #1
    sub x3, x3, #1
    b 1b
1:
    sub x0, x0, x10
    ret
