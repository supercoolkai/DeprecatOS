# Directory Controller

## dir_row struct format
src `src/fs/ext2/directoryController.h`

for `ls`'s output, `NAME_LEN` is hardcoded to 256

| off | field | type |
|---|---|---|
| 0 | `inode` | `uint32_t` |
| 4 | `name[256]` | `uint8_t[256]` |

## overview
basically the controller of directories, whatever information is necessary from directories, this gets it. 

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

### `static bool name_in(struct ext2_inode inode, const char *name, uint32_t *out)`
checks whether the given directory inode `inode` contains an `ext2_directory_entry` with name `name`. if so, returns `true` and sets `*out` to point to the entry's inode. otherwise returns `false` and sets `*out` to point to `INODE_ERROR`.

### `static bool dir_is_empty(struct ext2_inode inode)`
like `name_in()`, but instead scans to see whether it has any `ext2_directory_entry` besides `.` and `..`

### `bool dir_insert(uint32_t parent_inode_n, const char *name, uint32_t child_inode_n)`
does numerous checks to see whether the given `parent_inode_n` and `child_inode_n` are valid for insertion, then attempts to find a slot in the parent inode which fits the new entry that will be created. if it finds one, it attempts to add that inode to that directory, returning whether it succeeded or not. otherwise, it just returns `false`.

### `bool dir_remove(uint32_t parent_inode_n, const char *name, uint32_t *removed_inode_n)`
attempts to remove an inode with the given name `name` from the parent inode. then it looks through all the `ext2_directory_entry`s in the parent inode. if it ends up finding one with name `name`, then it attempts to remove it and returns the success (also sets `removed_inode_n` to the inode number of the removed entry). otherwise, it returns false as it didn't find any inode with that name.

### `bool make_dir(uint32_t parent_inode_n, const char *name)`
attempts to make a directory inside the parent directory at `parent_inode_n` and sets the new entry's name to `name` using the function `dir_insert()`. if it fails to insert (or the parent inode given is not valid), returns `false`, if it succeeds, returns `true`.

### `bool unlink_inode(uint32_t parent_inode_n, const char *name)`
unlinks the inode with name `name` inside the directory with inode number `parent_inode_n`. checks if `parent_inode_n` is valid, if not returns `false`. then, after the validity checks, attempts to call `dir_remove()` with the given parameters, then calls  `delete_inode()` on the returned inode that was removed from the directory. 
