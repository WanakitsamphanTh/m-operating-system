.section .rodata
    enter_el1_msg: .asciz "entered EL1\n"
    set_up_vec_table_msg: .asciz "initialized exception vector table\n"
    init_kernel_msg: .asciz "initialized kernel\n"
    
.text

.global _start
_start:
    /*initializing stack*/
    ldr x0, =STACK_TOP
    mov sp, x0

    /* mask interrupts */
    msr daifset, #0xf

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

    /* set up exception vector table */
    ldr x0, =el1_vector_table
    msr vbar_el1, x0
    isb

    ldr x0, =set_up_vec_table_msg
    bl printk

    /* init kernel platform */
    bl init_kernel

    ldr x0, =init_kernel_msg
    bl printk

    /* enable interrupts */
    msr daifclr, #0b0010

    /* enter kernel */
enter_kernel:
    bl kmain

.global kernel_halt
kernel_halt:
1:
    wfi
    b 1b    // jmp 1

/* Enable FP/NEON */
.global enable_fp_neon
enable_fp_neon:
    mrs x0, cpacr_el1
    orr x0, x0, #(3 << 20)
    msr cpacr_el1, x0
    isb
    ret