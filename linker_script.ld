ENTRY(_start)

SECTIONS
{
  . = 1M;

  .multiboot :
  {
    KEEP(*(.multiboot))
  }

  .text :
  {
    *(.text)
    *(.text.*)
  }

  .rodata :
  {
    *(.rodata)
    *(.rodata.*)
  }

  .data :
  {
    . = ALIGN(4);
    *(.data)
    *(.data.*)
    . = ALIGN(4);
  }

  .bss :
  {
    . = ALIGN(4);

    _sbss = .;

    *(.bss)
    *(.bss.*)
    *(COMMON)

    . = ALIGN(4);

    _ebss = .;
  }
}
