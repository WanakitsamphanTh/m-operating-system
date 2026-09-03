.equ GICD_BASE, 0x08000000
.equ GICD_CTLR, GICD_BASE + 0x00000000

.equ GICR_BASE, 0x080A0000
.equ GICR_WAKER, GICR_BASE + 0x0014
.equ GICR_SGI_BASE, GICR_BASE + 0x10000
.equ GICR_ISENABLER0, GICR_SGI_BASE + 0x0100

.section .rodata
    init_gic_msg: .asciz "initialized GIC\n"
    init_gicr_msg: .asciz "initialized GIC redistributor\n"
    init_timer_msg: .asciz "initialized timer\n"
    init_ppi_msg: .asciz "initialized PPI\n"
    enable_dist_msg: .asciz "enabled distributor\n"
    config_timer_msg: .asciz "configured timer\n"

.section .text.boot
.global init_kernel
init_kernel:
    stp x29, x30, [sp, #-16]!
    bl init_gic
    ldr x0, =init_gic_msg
    bl printk

    bl init_timer
    ldr x0, =init_timer_msg
    bl printk

    bl config_timer
    ldr x0, =config_timer_msg
    bl printk

    ldp x29, x30, [sp], #16
    ret

init_gic:
    stp x29, x30, [sp, #-16]!

    /* disable distributor */
    ldr x0, =GICD_CTLR
    mov w1, wzr
    str w1, [x0]

    dsb sy
    isb

    bl init_gic_redistributor
    ldr x0, =init_gicr_msg
    bl printk

    bl init_gic_cpu_interface
    ldr x0, =init_ppi_msg
    bl printk

    /* enable distributor */
    ldr x0, =GICD_CTLR
    mov w1, #(1 << 5) | (1 << 1) | (1 << 0)  /* ARE_NS | EnableGrp1S | EnableGrp1NS */
    str w1, [x0]
    isb

    ldr x0, =enable_dist_msg
    bl printk

    ldp x29, x30, [sp], #16
    ret

init_gic_redistributor:
    ldr x0, =GICR_WAKER
    ldr w1, [x0]
    /* Clear ProcessorSleep */
    bic w1, w1, #0b10
    str w1, [x0]
    /* wait until children is a sleep */
1:
    ldr w1, [x0]
    tbz w1, #2, 1f  /* test ChildrenAsleep == 0 */
  wfe
    b 1b
1:
    ret

init_gic_cpu_interface:
    /* Enable GIC system-register interface */
    mrs x0, ICC_SRE_EL1
    orr x0, x0, #1
    msr ICC_SRE_EL1, x0
    isb

    /* allow all normal interrupts priority */
    mov x0, #0xff
    msr ICC_PMR_EL1, x0
    /* Binary point */
    mov x0, #0
    msr ICC_BPR1_EL1, x0 
    /* Enable Group 1 interrupts */
    mov x0, #1
    msr ICC_IGRPEN1_EL1, x0
    dsb sy
    isb
    ret

init_timer:
    /* set PPI 27 (virtual timer) to Group 1 (IRQ, non-secure) */
    ldr x0, =GICR_SGI_BASE + 0x0080   /* GICR_IGROUPR0 */
    mov w1, #(1 << 27)
    str w1, [x0]

    ldr x0, =GICR_ISENABLER0
    ldr w1, [x0]
    orr w1, w1, #(1 << 27)   /* enable IRQ virtual timer */
    str w1, [x0]
    ret

.global config_timer
config_timer:
    /* load tick frequency [intervals/second] */
    mrs x0, CNTFRQ_EL0
    mov x2, #1000
    udiv x0, x0, x2

    /* set virtual timer countdown */
    msr CNTV_TVAL_EL0, x0

    /* Enable virtual timer  */
    mov x0, #1
    msr CNTV_CTL_EL0, x0

    isb
    ret

.global reset_timer
reset_timer:
    /* current deadline */
    mrs x0, CNTV_CVAL_EL0 
    /* load tick frequency [intervals/second] */
    mrs x1, CNTFRQ_EL0
    mov x2, #1000
    udiv x1, x1, x2
    add x0, x0, x1
    /* set ticks */
    msr CNTV_CVAL_EL0, x0

    isb
    ret