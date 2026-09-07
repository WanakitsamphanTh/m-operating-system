.section .text.boot

.global set_page_table
set_page_table:
    msr ttbr0_el1, x0
    ret

.global enable_mmu
enable_mmu:
    msr mair_el1, x0
    msr tcr_el1, x0
    msr ttbr0_el1, x1
    dsb sy
    isb

    mrs x0, sctlr_el1
    orr x0, x0, #1      /* M = 1 : enable MMU */
    orr x0, x0, #(1 << 5) /* C = 1 : enable operations barrier */
    orr x0, x0, #(1 << 12) /* I = 1 : cacheable */
    msr sctlr_el1, x0
    ret