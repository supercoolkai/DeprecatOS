# Syscall Controller

## purpose
link between ring 3 and ring 0, essentially the gateway between the userland and kernel. all syscalls are housed here

## global variables
**scratch:** a buffer array of size **BIT_32_PER_BLK** (see `docs/terms/definedTerms.md`)

## misc functions

### `resolve_dir(const char *s)`
attempts to lookup the given path using `lookup_path()` and returns the given inode_n if it succeeds.

## syscalls

### `sys_write_char(char c):`
write a given char from register `ebx` onto both serial and framebuffer with color **WHITE**

### `sys_write_string(char \*s)`
write a given string from register `ebx` (in the form of a pointer) onto both serial and framebuffer with color **WHITE**

### `sys_get_ticks(void)`
returns the current tick using `timer_get_ticks`

### `sys_exit_curr(void)`
kills the current process, panics if no other processes on the pqueue (see `docs/process/processQueue.md`)

### `sys_read_char(void)`
returns the current char from the rb (see `docs/util/ringBuffer.md`), if none then return **SENTINEL**

### `sys_yield(void)`
if there is a current process, push current process onto the back of the pqueue then pop the old one

### `sys_write_char_color(char c, unsigned char color)`
write a given char from register `ebx` onto both serial and framebuffer with the given color from `ecx`

### `sys_write_string_color(char *s, unsigned char color`
write a given string from register `ebx` (in the form of a pointer) onto both serial and framebuffer with the given color from `ecx`

### `sys_read_chunk(uint32_t inode_n, uint32_t chunk_n, uint32_t *buf)`
reads a 1 block chunk using the inode's correct indirect/direct block pointer (determined with chunk_n). returns the block data to buf

### `sys_resolve_dir(const char *s)`
attempts to resolve the given path using `resolve_dir()`, and returns the inode number given.

### `sys_write_string_len(uint32_t base, uint32_t len)`
if base is a pointer to a const char *, write parameter **len** characters from the string.

### `sys_get_stat(uint32_t ino_n, uint32_t *buf)`
returns the given inode_n's corresponding `struct ext2_inode` to *buf. 
