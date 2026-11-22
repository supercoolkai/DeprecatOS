# Syscall Controller

## syscall number -> syscall map

src: `user/sys/syscall.c` (`int 0x80`, number in EAX). 

| # | name | args (registers) | returns |
|---|---|---|---|
| 1 | `SYS_WRITE_CHAR` | ebx = char | eax = 0 |
| 2 | `SYS_WRITE_STR` | ebx = str | eax = 0 / error |
| 3 | `SYS_GET_TICKS` | ebx = `uint64_t *` tick_out | eax = 0 / error, ticks written to `*tick_out` |
| 4 | `SYS_EXIT` | | (no return) |
| 5 | `SYS_READ_CHAR` | | eax = char, `SENTINEL` if none |
| 6 | `SYS_YIELD` | | eax = 0 |
| 7 | `SYS_WRITE_CHAR_COLOR` | ebx = char, ecx = color | eax = 0 |
| 8 | `SYS_WRITE_STR_COLOR` | ebx = str, ecx = color | eax = 0 / error |
| 9 | `SYS_READ_CHUNK` | edi = fs, ebx = inode, edx = chunk#, ecx = buf | eax = bytes (0 = past EOF) / error |
| 10 | `SYS_RESOLVE_DIR` | ebx = path, edx = `uint32_t *` fs_out | eax = inode / error, fs index written to `*fs_out` |
| 11 | `SYS_WRITE_STR_LEN` | ebx = buf, ecx = len | eax = 0 / error |
| 12 | `SYS_GET_STAT` | edi = fs, ebx = inode, ecx = buf (128 B) | eax = 0 / error |
| 13 | `SYS_MAKE_DIR` | edi = fs, ebx = parent inode, ecx = name, edx = `uint32_t *` fs_out | eax = new inode / error |
| 14 | `SYS_REMOVE_INODE` | edi = fs, ebx = parent inode, ecx = name | eax = 0 / error |
| 15 | `SYS_REMOVE_DIR` | edi = fs, ebx = parent inode, ecx = name | eax = 0 / error |
| 16 | `SYS_GET_SCREEN_DIMS` | ebx = `uint32_t *` row_out, ecx = `uint32_t *` col_out | eax = 0 / error |
| 17 | `SYS_SET_CURSOR` | ebx = row, ecx = col | eax = 0 / error |
| 18 | `SYS_CLEAR_SCREEN` | | eax = 0 |
| 19 | `SYS_REPLACE_INODE` | edi = fs, ebx = inode, ecx = buf, edx = f_size | eax = 0 / error |
| 20 | `SYS_MAKE_INODE` | edi = fs, ebx = parent inode, ecx = name, edx = `uint32_t *` fs_out | eax = new inode / error |
| 21 | `SYS_SET_CURSOR_NO_UPD` | ebx = row, ecx = col | eax = 0 / error |
| 22 | `SYS_WRITE_CHAR_NO_UPD` | ebx = char | eax = 0 |
| 23 | `SYS_WRITE_STR_NO_UPD` | ebx = str | eax = 0 / error |
| 24 | `SYS_WRITE_STR_LEN_NO_UPD` | ebx = buf, ecx = len | eax = 0 / error |
| 25 | `SYS_SET_SCROLLBACK` | ebx = bool | eax = 0 |
| 26 | `SYS_GET_CURSOR` | ebx = `uint32_t *` row_out, ecx = `uint32_t *` col_out | eax = 0 / error |
| 27 | `SYS_GET_EPOCH` | ebx = `uint64_t *` epoch_out | eax = 0 / error |
| 28 | `SYS_RENAME_INODE` | edi = fs, ebx = old parent inode, ecx = old name, edx = new parent inode, esi = new name | eax = 0 / error |
| 29 | `SYS_APPEND_TO_INODE` | edi = fs, ebx = inode, ecx = buf, edx = f_size | eax = 0 / error |
| 30 | `SYS_MAKE_LINK` | edi = fs, ebx = inode, esi = parent fs, edx = parent inode, ecx = name | eax = 0 / error |
| 31 | `SYS_MAKE_SYMLINK` | edi = fs, ebx = parent inode, edx = name, ecx = target | eax = 0 / error |
| 32 | `SYS_RESOLVE_SYMLINK` | ebx = path, edx = `uint32_t *` fs_out | eax = inode / error, fs index written to `*fs_out` |
| 33 | `SYS_TRUNCATE_INODE` | edi = fs, ebx = inode, ecx = new size | eax = 0 / error |
| 34 | `SYS_INODE_IS_MOUNTPOINT` | ebx = fs, ecx = inode | eax = 1 / 0 / error |
| 35 | `SYS_MOUNT_FS` | ebx = label | eax = 0 / error |
| 36 | `SYS_UMOUNT_FS` | ebx = label | eax = 0 / error |
| 37 | `SYS_HALT` | | (no return) |

"error" is `SYSCALL_ERROR` (`0xFFFFFFFF`, `kernel/errors.h`). "fs" is an index into `fs_entries`, turned into a `struct fs_info *` by `fs_from_index()`. the handler side sees the registers as a pushad frame: `frame[0]` = edi, `[1]` = esi, `[2]` = ebp, `[3]` = esp, `[4]` = ebx, `[5]` = edx, `[6]` = ecx, `[7]` = eax. the userland wrappers (`user/sys/syscall.c`) are `write_char` ... `halt` in the same order as the table.

> `SYS_EXIT` is hardcoded as `.set SYS_EXIT, 4` in `user/shell/crt0.S`

---

## purpose
link between ring 3 and ring 0, essentially the gateway between the userland and kernel. all syscalls are housed here

## global variables
#### `static uint32_t scratch[BIT_32_PER_BLK]`:
a buffer array used in numerous syscalls, most notably `sys_read_chunk()`

#### `static uint32_t (*syscall_table[])(uint32_t *)`:
a table of syscalls which can be called through entering a syscall's index in the table alongside a frame.

## function analysis (excludes syscalls)

### `static uint32_t resolve_dir(const char *s, bool split_leaf_sym, struct fs_info **fs)`
bounds-checks the string against the user range, then looks the path up from `root_fs` using `lookup_path()`. on success returns the inode number and writes the filesystem that owns it to `*fs`, otherwise `SYSCALL_ERROR`. `split_leaf_sym` is passed through as `lookup_path()`'s `follow_leaf_link`, so `true` follows a trailing symlink and `false` returns the link's own inode.

### `static struct fs_info *fs_from_index(uint32_t index)`
turns a userland fs index into a `struct fs_info *`. returns `NULL` if the index is `>= MAX_MOUNTS`, or if the slot is not `root_fs` and has no mountpoint (nothing mounted there).

### `void syscall_init(void)`
sets up the IDT gate at `SYSCALL_VECTOR_NUMBER` with attribute bytes `0xEE` (present, ring 3, type interrupt gate), and stub `syscall_stub`

### `uint32_t syscall_handler(uint32_t esp)`
called by a syscall IRQ, if the function # is valid then it calls/returns the function at `syscall_table[function_n]` with the provided frame as the parameter

## syscalls
do note that all syscalls return `(uint32_t) frame`

### `sys_write_char(char c):`
write a given char from register `ebx` onto both serial and framebuffer with color **WHITE**

### `sys_write_string(char \*s)`
write a given string from register `ebx` (in the form of a pointer) onto both serial and framebuffer with color **WHITE**

### `sys_get_ticks(void)`
returns the current tick using `timer_get_ticks`

### `sys_exit_curr(void)`
kills the current process, panics if no other processes on the pqueue (see `docs/process/queue.md`)

### `sys_read_char(void)`
returns the current char from the rb (see `docs/etc/lib/ringBuffer.md`), if none then return **SENTINEL**

### `sys_yield(void)`
if there is a current process, push current process onto the back of the pqueue then pop the old one

### `sys_write_char_color(char c, unsigned char color)`
write a given char from register `ebx` onto both serial and framebuffer with the given color from `ecx`

### `sys_write_string_color(char *s, unsigned char color)`
write a given string from register `ebx` (in the form of a pointer) onto both serial and framebuffer with the given color from `ecx`

### `sys_read_chunk(uint32_t fs, uint32_t inode_n, uint32_t chunk_n, uint32_t *buf)`
reads a 1 block chunk using the inode's correct indirect/direct block pointer (determined with chunk_n). returns the block data to buf

### `sys_resolve_dir(const char *c, uint32_t *fs_out)`
attempts to resolve the given path using `resolve_dir()` (following a trailing symlink), returns the inode number and writes the index of the filesystem that owns it to `*fs_out`.

### `sys_write_string_len(uint32_t base, uint32_t len)`
if base is a pointer to a const char *, write parameter **len** characters from the string.

### `sys_get_stat(uint32_t fs, uint32_t ino_n, uint32_t *buf)`
returns the given inode_n's corresponding `struct ext2_inode` to `*buf`. 

### `sys_make_dir(uint32_t fs, uint32_t inode_n, const char *name, uint32_t *fs_out)`
makes a new empty directory using `make_dir()`

### `sys_remove_inode(uint32_t fs, uint32_t inode_n, const char *name)`
attempts to remove a given inode using `unlink_inode()`

### `sys_remove_dir(uint32_t fs, uint32_t inode_n, const char *name)`
attempts to remove a given directory using `unlink_dir()`

### `sys_get_screen_dims(uint32_t *row_out, uint32_t *col_out)`
hands the screen dimensions using `fb_get_screen_dims()`

### `sys_set_cursor(uint32_t row, uint32_t col)`
sets the current cursor to the given coordinates if in bounds using `fb_set_cursor()`

### `sys_clear_screen(void)`
clears the screen using `fb_clear_screen()`

### `sys_replace_inode(uint32_t fs, uint32_t inode_n, uint16_t *buf, uint32_t f_size)`
replaces the contents of an inode with the given `*buf`'s contents, using `replace_inode()`

### `sys_make_inode(uint32_t fs, uint32_t inode_n, const char *name, uint32_t *fs_out)`
makes a new empty file using `make_file()`

### `sys_set_cursor_no_upd(uint32_t row, uint32_t col)`
sets the current cursor to the given cursor if in bounds using `fb_set_cursor_no_upd()`, doesn't move the cursor on screen

### `sys_write_char_no_upd(char c)`
writes a given char using `serial_write_char()` and `fb_draw_char_no_upd()`, doesn't update the cursor

### `sys_write_string_no_upd(char *c)`
writes a given string by repeatedly using `serial_write_char()` and `fb_draw_char_no_upd()`, doesn't update the cursor

### `sys_write_string_len_no_upd(const char *buf, uint32_t len)`
writes the first `len` characters from a given string by repeatedly using `serial_write_char()` and `fb_draw_char_no_upd()`, doesn't update the cursor

### `sys_set_scrollback(bool v)`
sets the framebuffer's scrollback to a given `bool` value using `fb_set_scrollback()`

### `sys_get_cursor(uint32_t *row, uint32_t *col)`
hands the current framebuffer cursor's coordinates back using `fb_get_cursor()`

### `sys_get_epoch(uint64_t *epoch_out)`
hands back the current epoch using `timer_get_epoch()`

### `sys_rename_inode(uint32_t fs, uint32_t old_parent_n, uint32_t new_parent_n, const char *old_name, const char *new_name)`
renames/moves a given inode to a `new_parent_n` using `rename_inode()`

### `sys_append_to_inode(uint32_t fs, uint32_t inode_n, uint16_t *buf, uint32_t f_size)`
appends a given `buf` to a given `inode_n` using `append_to_inode()`

### `sys_link_inode(uint32_t fs, uint32_t inode_n, uint32_t parent_fs, uint32_t parent_n, const char *name)`
make a new hard link to a given `inode_n`

### `sys_make_symlink(uint32_t fs, uint32_t parent_n, const char *name, const char *target)`
makes a new symbolic link to a given `target`

### `sys_resolve_symlink(const char *c, uint32_t *fs_out)`
resolves the given path using `resolve_dir()` WITHOUT following a trailing symlink, so if the last component is a symlink it returns the link's own inode (everything before it is still followed). writes the owning fs index to `*fs_out`

### `sys_truncate_inode(uint32_t fs, uint32_t inode_n, uint32_t new_size)`
truncates an `inode_n` to a `new_size` using `truncate_inode()`

### `sys_inode_is_mountpoint(uint32_t fs, uint32_t inode_n)`
returns whether an `inode_n` is a mountpoint using `return_mnt_root()`

### `sys_mount_fs(const char *name)`
mounts a given filesystem to its mountpoint using `mount_fs()`

### `sys_umount_fs(const char *name)`
umounts a given mounted filesystem using `umount_fs()`

### `sys_halt(void)`
follows the halt sequence by umounting every mounted filesystem with `umount_fs()`, marking the root_fs as usable with `mark_fs_state()`, then flushing all drives with `ata_flush_drives()`
