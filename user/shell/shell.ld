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

ASSERT(. <= 0x40100000, "shell.bin exceeds the 1 MiB user code region (256 pages before STACK_ADDR)")

/DISCARD/ : {
  *(.eh_frame)
  *(.note.*)
  *(.got*)
  *(.comment)}
}
