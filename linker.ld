ENTRY(_start)

SECTIONS
{
    . = 0x40080000;
    KERNEL_START = .;
    .boot : {
        KEEP(*(.text.boot*))
    }

    .vectors  ALIGN(2048) : {
        KEEP(*(.text.vector_table*))
    }
    .text ALIGN(16) : {
        *(.text)
    }
    KERNEL_END = .;
    KERNEL_SIZE = . - KERNEL_START;
    
    RODATA_START = .;
    .rodata ALIGN(16) : {
        *(.rodata*)
    }
    RODATA_END = .;
    RODATA_SIZE = . - RODATA_START;

    .data ALIGN(16) : {
        *(.data*)
    }

    .bss ALIGN(16) : {
        *(.bss*)
        *(COMMON)

        . = ALIGN(16);

        STACK_BOTTOM = .;
        . += 0x10000; 
        STACK_TOP = .;
    }
}