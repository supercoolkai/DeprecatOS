# Directory Controller

## purpose
basically the controller of directories, whatever information is necessary from directories, this gets it. 

## global variables
#### `static uint16_t blk_buf[BLOCK_SIZE / 2 * INODE_BLK_PTR_AMT]`:
a table which lookup_path() uses (soon to be replaced with a kmalloc)

## function analysis 
### `struct ext2_directory_entry *return_next_dir_entry(uint16_t *buf, uint32_t *pos)`
advances the cursor by the current entry size and returns the new entry at that position

### `bool ls_dir(uint16_t *buf, struct dir_row *out, uint32_t size, uint32_t max)`
iterates through directory `buf` until it reaches the end, and adds each entry it iterates through onto `out`. returns a bool depending on its success with this process.

### `static bool comp_name(const uint8_t *a, uint8_t len, const char *b)`
compares two `const uint8_t *`s to see whether they are equal. returns true if yes, false if no.

### `static bool lookup(uint16_t *buf, const char *name, uint32_t *out, uint32_t size)`
iterates through a given directory inode to see whether an inode with name `name` is in it. return true if yes, false if no.

### `bool lookup_path(const char *path, uint32_t *out)`
checks to see whether the path exists, if it does read the inode/directory into `out` and return true, if not return false.
