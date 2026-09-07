ENTRY(_start)

SECTIONS
{
    . = 0x40080000;
    KERNEL_START = .;
    KERNEL_TEXT_START = .;
    .boot : {
        KEEP(*(.text.boot*))
    }

    .vectors  ALIGN(2048) : {
        KEEP(*(.text.vector_table*))
    }
    .text ALIGN(16) : {
        *(.text)
    }
    KERNEL_TEXT_END = .;
    KERNEL_TEXT_SIZE = . - KERNEL_TEXT_START;
    
    KERNEL_RODATA_START = .;
    .rodata ALIGN(16) : {
        *(.rodata*)
    }
    KERNEL_RODATA_END = .;
    KERNEL_RODATA_SIZE = . - KERNEL_RODATA_START;

    KERNEL_DATA_START = .;
    .data ALIGN(16) : {
        *(.data*)
    }

    .bss ALIGN(16) : {
        *(.bss*)
        *(COMMON)
    }

    KERNEL_DATA_END = .;
    KERNEL_DATA_SIZE = . - KERNEL_DATA_START;

    . = ALIGN(16);
    STACK_BOTTOM = .;
    . += 0x100000; 
    STACK_TOP = .;

    KERNEL_END = .;
    KERNEL_SIZE = . - KERNEL_START; 
}