ENTRY(_start)

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

ASSERT(. <= 0x40010000, "shell.bin exceeds the 64 KiB user code region (16 pages before STACK_ADDR)")

/DISCARD/ : {
  *(.eh_frame)
  *(.note.*)
  *(.got*)
  *(.comment)}
}
