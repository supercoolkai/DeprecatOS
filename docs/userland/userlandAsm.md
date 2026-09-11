# Userland ASM Section

## globals
#### `user_prog_start`:
the start of the userland init function

#### `user_prog_end`:
the end of the function in `user_prog_start`

## overview
starts up the userland programs, a parallel to `mainInitScript.c` in the kernel.

## function analysis

### `user_prog`
starts up the user programs, currently only includes `shell.bin`, or the shell.
