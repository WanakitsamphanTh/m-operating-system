ENTRY(_start)

SECTIONS
{
    . = 0x40080000;
    .text : {
        *(.text*)
    }
    .rodata : {
        *(.rodata*)
    }
    .data : {
        *(.data*)
    }
    .bss : {
        *(.bss*)
        *(COMMON)

        . = ALIGN(16);

        STACK_BOTTOM = .;
        . += 0x1000; 
        STACK_TOP = .;
    }
}