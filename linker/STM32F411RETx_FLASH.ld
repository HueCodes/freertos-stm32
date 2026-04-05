/*
 * Linker script for STM32F411RETx
 *
 * Memory map:
 *   FLASH: 512KB at 0x08000000
 *     Sectors 0-5 (384KB): application code
 *     Sector 6   (128KB):  reserved for config storage (see flash_config.h)
 *   RAM:   128KB at 0x20000000
 *
 * How this file works:
 *   MEMORY block tells the linker what memory regions exist and their sizes.
 *   SECTIONS block tells the linker where to place each type of data.
 *
 *   At reset, the CPU starts at the reset handler address stored in the
 *   vector table at 0x08000000. The startup code then:
 *     1. Copies .data from flash to RAM
 *     2. Zeroes .bss in RAM
 *     3. Calls static constructors
 *     4. Calls main()
 */

ENTRY(Reset_Handler)

/* Total RAM used by stack. FreeRTOS tasks have their own stacks in the heap.
   This _estack value is for the initial/MSP stack used before the scheduler starts. */
_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Heap_Size  = 0x400;    /* Minimum heap for newlib (not FreeRTOS heap) */
_Min_Stack_Size = 0x400;    /* Initial stack before scheduler starts */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 384K   /* sectors 0-5, application */
    CONFIG (rx) : ORIGIN = 0x08060000, LENGTH = 128K   /* sector 6, config storage */
    RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    /* Vector table and code goes first in flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))    /* KEEP prevents gc-sections from removing it */
        . = ALIGN(4);
    } >FLASH

    /* Application code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)              /* ARM/Thumb interworking glue */
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } >FLASH

    /* Read-only data (const variables) */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* ARM exception handling tables */
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >FLASH

    /* C++ constructors/destructors */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } >FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } >FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } >FLASH

    /* Initialised data: stored in flash, copied to RAM at startup.
       _sidata = where it lives in flash. _sdata/_edata = where it goes in RAM. */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT>FLASH

    /* Zero-initialised data: not stored in flash, startup code clears to 0. */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } >RAM

    /* User heap and stack - check there is enough RAM */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM

    /* Discard C++ exception handling if not used */
    /DISCARD/ :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
