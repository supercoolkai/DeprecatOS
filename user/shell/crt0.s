.equ SYS_EXIT, 4

movw $0x23, %ax
movw %ax, %ds
movw %ax, %es
movw %ax, %fs
movw %ax, %gs

call main

movl $SYS_EXIT, %eax
int $0x80


.section .note.GNU-stack, "", @progbits
