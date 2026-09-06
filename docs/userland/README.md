# Userland 

## overview 
in the kernel, contains all things to do with the userland 

### `userland`
setup for any actions in the userland, must load before any syscalls. maps stack, code, and misc pages. 

### `syscall`
link between ring 0 and ring 3, contains all syscalls accessible from `user/`
