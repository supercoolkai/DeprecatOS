# Directory Controller

## purpose
basically the controller of directories, whatever information is necessary from directories, this gets it. 

## global variables
#### **blk_buf:** a table which lookup_path() uses (soon to be replaced with a kmalloc)

## function analysis 
### `struct ext2_directory_entry *return_next_dir_entry(uint16_t *buf, uint32_t *pos)`
advances the cursor by the current entry size and returns the new entry at that position

### `bool ls_dir(uint16_t *buf, struct dir_row *out, uint32_t size, uint32_t max)`
iterates through directory `buf` until it reaches the end, and adds each entry it iterates through onto `out`. returns a bool depending on its success with this process.

### `bool lookup_path(const char *path, uint32_t *out)`
checks to see whether the path exists, if it does read the inode/directory into `out` and return true, if not return false
