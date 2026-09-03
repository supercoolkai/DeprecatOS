.section .text
.global user_prog_start
.global user_prog_end
user_prog_start:
    .incbin "shell.bin"
user_prog_end:



.section .note.GNU-stack, "", @progbits
