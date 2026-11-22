# depsh

## shared buffers

#### `extern uint32_t fs_buf[BIT_32_PER_BLK]`:
a buffer used in reading functions like `cat` to store `read_chunk()`'s output.

#### `extern uint8_t stat_buf[128]`:
basically the same thing as `fs_buf[]`, but used to store `get_stat()`'s output

#### `extern char dir[256]`:
stores the current working directory (CWD)

## utilities

### `const char *return_path(char *args)`
turns the shell's `args` into an absolute path. if `args` already starts with `/` it's returned as-is, otherwise `dir` + `/` + `args` is built into a file-scope static buffer (not `dir`) and that's returned. returns 0 if the result wouldn't fit in 255 bytes

### `bool is_dir(uint32_t inode_n, uint32_t fs)`
returns whether the given `inode_n` is a directory

### `bool split_parent_leaf(char *abs_path, uint32_t *parent_n_out, char **leaf_out, uint32_t *fs_out)`
takes a path returned from `return_path()` and splits it into: parent inode_n, leaf name, and the filesystem its under


## commands

`<arg>` is required, `[arg]` is optional

### `help`
displays a short description of every command

### `echo [text ...] [> <path> | >> <path>]`
displays the given text, if given `>` or `>>` it expects a path and it will print to that path. creates new files if nonexistent, but will not create directories. also fails on a missing parent directory with "when attempting to create missing file, an unexpected error occurred when splitting the path"

### `ticks`
displays the current amount of ticks accumulated since boot

### `cat <path>`
reads an inode at a given path, warns if it's a nonexistent path or its a directory

### `ls [path]`
lists the given directory, rejects if invalid path or not a directory

### `cd [path]`
changes the current working directory to the given path, rejects if the path has a file in it or its a non-real path

### `stat <-t|-s|-l> <path>`
displays the stats of the given path, either type, size, or links

### `mkdir [-p] <path>`
makes a new directory, if given the flag `-p`, then makes the whole path. otherwise just makes the leaf and if nonexistent parent then rejects.

### `rm [-r] <path>`
removes the given path, if `-r` is given then it accepts directories and will recursively remove all files in it. otherwise it rejects directories.

### `clear`
clears the screen

### `touch <path>`
creates a new file at the path, rejects if the given path has a file in the parent, the path already exists, or its a nonexistent parent path

### `date`
prints the current date and time as stored in the RTC (no timezone handling, so it's whatever the BIOS clock is set to)

### `mv <old_path> <new_path>`
moves the file/directory from `old_path` to `new_path`, can rename also. doesnt move across filesystems, and refuses if `new_path` already exists or if `old_path` doesn't exist

### `cp [-r] <old_path> <new_path>`
copies the file/directory from `old_path` to `new_path`, only accepts directories if `-r` is given though. rejects if `old_path` is nonexistent or `new_path` is not real, and refuses if `new_path` is inside `old_path`. also won't copy a mountpoint during a recursive copy

### `ln [-s] <path> <link_path>`
creates a link to `path`, with its path being `link_path`. if not given `-s` as a flag, then it rejects if `path` is not real. rejects if `link_path`'s parent path does not exist no matter what. also if there is no `-s` flag, it rejects directories.

### `mount <label>`
mounts the given filesystem, rejects if its already mounted or it is not a real filesystem

### `umount <label>`
umounts the given filesystem, rejects if its already umounted or it is not a real filesystem. refuses with "busy dir" if the current working directory is under `/mnt/<label>`

### `halt`
umounts all the filesystems and makes it safe to shutdown
