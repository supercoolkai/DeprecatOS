SECTIONS 
{
. = 0x40000000;

.text :
{
  *(.text)
  *(.rodata*)
  *(.data*)
  *(.bss)
  *(COMMON)
}

ASSERT(. <= 0x40004000, "shell.bin exceeds the 16 KiB user code region (4 pages before STACK_ADDR)")

/DISCARD/ : {
  *(.eh_frame)
  *(.note.*)
  *(.got*)
  *(.comment)}
}
