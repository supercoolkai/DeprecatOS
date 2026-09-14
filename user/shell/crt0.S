#include "gdt/segments.h"
.set SYS_EXIT, 4

.global _start
_start:
movw $USER_DATA_SEGMENT_SEL, %ax
movw %ax, %ds
movw %ax, %es
movw %ax, %fs
movw %ax, %gs

call main

movl $SYS_EXIT, %eax
int $0x80


.section .note.GNU-stack, "", @progbits
