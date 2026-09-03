ENTRY(_start)

SECTIONS
{
    . = 0x40080000;
    .boot : {
        KEEP(*(.text.boot*))
    }
    . = ALIGN(2048);
    .vectors : {
        KEEP(*(.text.vector_table*))
    }
    .text : {
        *(.text)
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
        . += 0x10000; 
        STACK_TOP = .;
    }
}