.section .text.boot
.equ CTX_SIZE, 272
.macro save_context elr, spsr, esr
    sub sp, sp, #CTX_SIZE
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    mrs x0, \elr
    stp x30, x0, [sp, #240]
    mrs x0, \spsr
    mrs x1, \esr
    stp x0, x1, [sp, #256]
    mov x0, sp
.endm

.macro restore_context elr, spsr, esr
    mov sp, x0
    ldp x0, x1, [sp, #0]
    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    ldp x8, x9, [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldp x30, x0, [sp, #240]
    msr \elr, x0
    ldp x0, x1, [sp, #256]
    msr \spsr, x0
    msr \esr, x1
    add sp, sp, #CTX_SIZE
.endm

/* ================== mischellaneous ===================== */

.global get_irq
get_irq:
    mrs x0, icc_iar1_el1
    ret

.global end_irq
end_irq:
    msr icc_eoir1_el1, x0
    isb
    ret

/* ==================== return from exception ==================== */
return_from_el1:
    restore_context elr_el1, spsr_el1, esr_el1
    eret

return_from_el2:
    restore_context elr_el2, spsr_el2, esr_el2
    eret

return_from_el3:
    restore_context elr_el3, spsr_el3, esr_el3
    eret

/* ==================== EL1 ==================== */
.section .text.vector_table
.align 11   /* 2^11 bytes align */
.global el1_vector_table
el1_vector_table:
el_sp0_el1: 
    save_context elr_el1, spsr_el1, esr_el1
    bl sync_sp0
    b return_from_el1

    .org el1_vector_table + 0x080
    save_context elr_el1, spsr_el1, esr_el1
    bl irq_sp0
    b return_from_el1

    .org el1_vector_table + 0x100
    save_context elr_el1, spsr_el1, esr_el1
    bl fiq_sp0
    b return_from_el1

    .org el1_vector_table + 0x180
    save_context elr_el1, spsr_el1, esr_el1
    bl serr_sp0
    b kernel_panic

    .org el1_vector_table + 0x200
el_spx_el1:
    save_context elr_el1, spsr_el1, esr_el1
    bl sync_spx
    b return_from_el1
    .org el1_vector_table + 0x280
    save_context elr_el1, spsr_el1, esr_el1
    bl irq_spx
    b return_from_el1
    .org el1_vector_table + 0x300
    save_context elr_el1, spsr_el1, esr_el1
    bl fiq_spx
    b return_from_el1
    .org el1_vector_table + 0x380
    save_context elr_el1, spsr_el1, esr_el1
    bl serr_spx
    b kernel_panic

lower64_el1:
    .org el1_vector_table + 0x400
    bl sync_l64
    .org el1_vector_table + 0x480
    bl irq_l64
    .org el1_vector_table + 0x500
    bl fiq_l64
    .org el1_vector_table + 0x580
    bl serr_l64

lower32_el1:
    .org el1_vector_table + 0x600
    bl sync_l32
    .org el1_vector_table + 0x680
    bl irq_l32
    .org el1_vector_table + 0x700
    bl fiq_l32
    .org el1_vector_table + 0x780
    bl serr_l32

/* ==================== EL2 ==================== */
.align 11   /* 2^11 bytes align */
.global el2_vector_table
el2_vector_table:
el_sp0_el2:
    .align 7
    save_context elr_el2, spsr_el2, esr_el2
    bl sync_sp0
    b return_from_el2
    .align 7
    save_context elr_el2, spsr_el2, esr_el2
    bl irq_sp0
    b return_from_el2
    .align 7
    save_context elr_el2, spsr_el2, esr_el2
    bl fiq_sp0
    b return_from_el2
    .align 7
    save_context elr_el2, spsr_el2, esr_el2
    bl serr_sp0
    b kernel_panic

el_spx_el2:
    .align 7
    b sync_spx
    .align 7
    b irq_spx
    .align 7
    b fiq_spx
    .align 7
    b serr_spx

lower64_el2:
    .align 7
    b sync_l64
    .align 7
    b irq_l64
    .align 7
    b fiq_l64
    .align 7
    b serr_l64

lower32_el2:
    .align 7
    b sync_l32
    .align 7
    b irq_l32
    .align 7
    b fiq_l32
    .align 7
    b serr_l32


/* ==================== EL3 ==================== */
.align 11   /* 2^11 bytes align */
.global el3_vector_table
el3_vector_table:
el_sp0_el3:
    .align 7
    save_context elr_el3, spsr_el3, esr_el3
    bl sync_sp0
    b return_from_el3
    .align 7
    save_context elr_el3, spsr_el3, esr_el3
    bl irq_sp0
    b return_from_el3
    .align 7
    save_context elr_el3, spsr_el3, esr_el3
    bl fiq_sp0
    b return_from_el3
    .align 7
    save_context elr_el3, spsr_el3, esr_el3
    bl serr_sp0
    b kernel_panic

el_spx_el3:
    .align 7
    b sync_spx
    .align 7
    b irq_spx
    .align 7
    b fiq_spx
    .align 7
    b serr_spx

lower64_el3:
    .align 7
    b sync_l64
    .align 7
    b irq_l64
    .align 7
    b fiq_l64
    .align 7
    b serr_l64

lower32_el3:
    .align 7
    b sync_l32
    .align 7
    b irq_l32
    .align 7
    b fiq_l32
    .align 7
    b serr_l32
