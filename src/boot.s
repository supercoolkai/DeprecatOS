.set MB_MAGIC,    0x1BADB002
.set MB_FLAGS,    0x6
.set MB_CHECKSUM, -(MB_MAGIC + MB_FLAGS)
.set MB_HEADER_ADDR, 0
.set MB_LOAD_ADDR, 0
.set MB_LOAD_END_ADDR, 0
.set MB_BSS_END_ADDR, 0
.set MB_ENTRY_ADDR, 0
.set DISPLAY_MODE_TYPE, 0
.set DISPLAY_WIDTH, 1024
.set DISPLAY_HEIGHT, 768
.set DISPLAY_DEPTH, 32

.section .multiboot, "a", @progbits
.align 4
    .long MB_MAGIC
    .long MB_FLAGS
    .long MB_CHECKSUM
    .long MB_HEADER_ADDR
    .long MB_LOAD_ADDR
    .long MB_LOAD_END_ADDR
    .long MB_BSS_END_ADDR
    .long MB_ENTRY_ADDR
    .long DISPLAY_MODE_TYPE
    .long DISPLAY_WIDTH
    .long DISPLAY_HEIGHT
    .long DISPLAY_DEPTH

.section .bss
.global stack_top
.align 16
stack_bottom:
    .skip 16384
stack_top:

.section .rodata
.global font8x16
font8x16:
    .incbin "drivers/fb/font8x16.bin"
.align 8
.global gdt
gdt:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
    .quad 0x00CFFA000000FFFF
    .quad 0x00CFF2000000FFFF
    .quad 0
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt - 1
    .long gdt

.section .text
.global _start
.extern mainInitScript
.extern timer_irq_handler
.extern keyboard_irq_handler
.extern exception_handler
.extern syscall_handler

_start:
    cli

    lgdt gdt_descriptor
    ljmp $0x08, $reload_segments
reload_segments:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $stack_top, %esp

zero_bss:
    movl $_sbss, %edi
    movl $_ebss, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb

startup_done:
    push %ebx
    call mainInitScript

hang:
    hlt
    jmp hang

.global irq0_stub
irq0_stub:
    pusha
    push %esp
    call timer_irq_handler
    movl %eax, %esp
    popa
    iret

.global irq1_stub
irq1_stub:
    pusha
    call keyboard_irq_handler
    popa
    iret

.global syscall_stub
syscall_stub:
    pusha
    push %esp
    call syscall_handler
    movl %eax, %esp
    //add $4, %esp
    popa
    iret

.global default_isr_stub
default_isr_stub:
    cli
    hlt
    jmp default_isr_stub

.macro exc_noerr n
.global exc\n
exc\n:
  push $0
  push $\n
  pusha
  push %esp
  call exception_handler
  movl %eax, %esp
  popa
  iret
.endm

.macro exc_err n 
.global exc\n
exc\n:
  push $\n
  pusha
  push %esp
  call exception_handler
  movl %eax, %esp
  popa
  iret
.endm

.global paging_enable
paging_enable:
  movl 4(%esp), %eax
  mov %eax, %cr3
  movl %cr0, %ecx
  orl $0x80000000, %ecx
  movl %ecx, %cr0
  ret

.global read_cr2
read_cr2:
  movl %cr2, %eax
  ret


exc_noerr 0
exc_noerr 1
exc_noerr 2
exc_noerr 3
exc_noerr 4
exc_noerr 5
exc_noerr 6
exc_noerr 7
exc_err 8    # fault x2 
exc_noerr 9
exc_err 10  #invalid tss 
exc_err 11 #nonexistent segment
exc_err 12 # stack-segment fault 
exc_err 13 #general error 
exc_err 14 #page fault 
exc_noerr 15
exc_noerr 16
exc_err 17 # alignemnt check, basically unreachable rn
exc_noerr 18
exc_noerr 19
exc_noerr 20
exc_err 21 # control protection, unreachable rn
exc_noerr 22
exc_noerr 23
exc_noerr 24
exc_noerr 25
exc_noerr 26
exc_noerr 27
exc_noerr 28
exc_noerr 29
exc_noerr 30
exc_noerr 31

.section .note.GNU-stack, "", @progbits
