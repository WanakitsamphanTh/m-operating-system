.section .rodata
    .align 3
    .global _kernel_start_addr
    .global _kernel_end_addr
    .global _kernel_size
    .global _kernel_text_start_addr
    .global _kernel_text_end_addr
    .global _kernel_text_size
    .global _kernel_rodata_start_addr
    .global _kernel_rodata_end_addr
    .global _kernel_rodata_size
    .global _kernel_data_start_addr
    .global _kernel_data_end_addr
    .global _kernel_data_size

    _kernel_start_addr: .quad KERNEL_START
    _kernel_end_addr: .quad KERNEL_END
    _kernel_size: .quad KERNEL_SIZE
    _kernel_text_start_addr: .quad KERNEL_START
    _kernel_text_end_addr: .quad KERNEL_END
    _kernel_text_size: .quad KERNEL_TEXT_SIZE
    _rodata_start_addr: .quad KERNEL_RODATA_START
    _rodata_end_addr: .quad KERNEL_RODATA_END
    _rodata_size: .quad KERNEL_RODATA_SIZE
    _data_start_addr: .quad KERNEL_DATA_START
    _data_end_addr: .quad KERNEL_DATA_END
    _data_size: .quad KERNEL_DATA_SIZE

    enter_el1_msg: .asciz "entered EL1\n"
    set_up_vec_table_msg: .asciz "initialized exception vector table\n"
    init_kernel_msg: .asciz "initialized kernel\n"
    enable_fp_msg: .asciz "enabled FP/NEON\n"

.section .bss
    .align 3
    .global dtb_boot
    dtb_boot: .quad 0

.section .text.boot
.global _start
_start:
    b _boot_start /* code0 */
    .word 0     /* code1 */
    .quad 0x80000
    .quad KERNEL_SIZE
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .word 0x644d5241 /* magic number ARMx64 */
    .word 0

_boot_start:

    /* store DTB in a callee-saved register */
    ldr x1, =dtb_boot
    str x0, [x1]

    /*initializing stack*/
    ldr x0, =STACK_TOP
    mov sp, x0

    /* mask interrupts */
    bl mask_interrupt

    /* determine current EL */
    mrs x0, CurrentEL
    lsr x0, x0, 2
    
    cmp x0, #0x01
    b.eq el1_boot
    cmp x0, #0x02
    b.eq el2_boot
    cmp x0, #0x03
    b.eq el3_boot

    b kernel_halt

el2_boot:
    /* enable GIC system register interface at EL2 first */
    mrs x0, ICC_SRE_EL2
    orr x0, x0, #(1 << 3)   /* DFB: disable FIQ bypass */
    orr x0, x0, #(1 << 0)   /* SRE: enable system register interface */
    msr ICC_SRE_EL2, x0
    isb

    /* lower to el1 */
    ldr x0, =STACK_TOP
    msr sp_el1, x0
    /* set EL1 entry */
    ldr x0, =el1_boot
    msr elr_el2, x0
    mov x0, #0b0101
    msr spsr_el2, x0
    msr hcr_el2, xzr
    isb
    eret

el3_boot:
    b kernel_halt

/* el1_boot */
el1_boot:
    ldr x0, =enter_el1_msg
    bl printk

    ldr x1, =dtb_boot
    ldr x0, [x1] 
    bl kmain

    b kernel_halt

.global init_exception_vector
init_exception_vector:
    stp x29, x30, [sp, #-16]!
    /* set up exception vector table */
    ldr x0, =el1_vector_table
    msr vbar_el1, x0
    isb

    ldr x0, =set_up_vec_table_msg
    bl printk

    ldp x29, x30, [sp], #16
    ret

.global enable_fp_neon
enable_fp_neon:
    stp x29, x30, [sp, #-16]!
    /* enable fp/neon */
    mrs x0, cpacr_el1
    orr x0, x0, #(3 << 20)
    msr cpacr_el1, x0
    isb
    
    ldr x0, =enable_fp_msg
    bl printk

    ldp x29, x30, [sp], #16
    ret
    
.global unmask_interrupt
unmask_interrupt:
    /* enable interrupts */
    msr daifclr, #0b0010
    ret

.global mask_interrupt
mask_interrupt:
    /* mask interrupts */
    msr daifset, 0xf
    ret

.global kernel_halt
kernel_halt:
1:
    wfi
    b 1b    // jmp 1
