.section .text.boot

.global enable_mmu
enable_mmu:
    msr mair_el1, x0

    /* TCR_EL1.IPS (bits[34:32]) = ID_AA64MMFR0_EL1.PARange (bits[3:0]) */
    mrs x3, id_aa64mmfr0_el1
    and x3, x3, #0xf
    lsl x3, x3, #32
    orr x1, x1, x3
    msr tcr_el1, x1

    msr ttbr0_el1, x2
    dsb sy
    isb

    mrs x0, sctlr_el1
    orr x0, x0, #1      /* M = 1 : enable MMU */
    orr x0, x0, #(1 << 2) /* C = 1 : enable data cache */
    orr x0, x0, #(1 << 12) /* I = 1 : enable instruction cache */
    msr sctlr_el1, x0
    ret