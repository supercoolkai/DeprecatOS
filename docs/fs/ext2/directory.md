# Directory Controller

## dir_row struct format
src `kernel/fs/ext2/directoryController.h`

for `ls`'s output, `NAME_LEN` is hardcoded to 256

| off | field | type |
|---|---|---|
| 0 | `inode` | `uint32_t` |
| 4 | `name[256]` | `uint8_t[256]` |

## overview
basically the controller of directories, whatever information is necessary from directories, this gets it. 

## function analysis 
### `static struct ext2_directory_entry *return_next_dir_entry(uint16_t *buf, uint32_t *pos)`
advances the cursor by the current entry size and returns the new entry at that position

### `static bool comp_name(const uint8_t *a, uint8_t len, const char *b)`
compares two `const uint8_t *`s to see whether they are equal. returns true if yes, false if no.

### `static bool lookup(uint16_t *buf, const char *name, uint32_t *out, uint32_t size)`
iterates through a given directory inode to see whether an inode with name `name` is in it. return true if yes, false if no.

### `bool lookup_path(struct fs_info *fs, const char *path, uint32_t *out, bool follow_leaf_link, struct fs_info **fs_out)`
checks to see whether the absolute `path` exists, if it does put the inode number into `out`, the filesystem it ended up in into `*fs_out` and return true, if not return false. walks from `root_fs` and crosses mountpoints in both directions on the way, splicing symlinks up to `SYMLINK_MAX_DEPTH` times. if `follow_leaf_link` is `false` and the last component is a symlink, the symlink's own inode is returned instead of what it points at.

### `static bool name_in(struct fs_info *fs, struct ext2_inode inode, const char *name, uint32_t *out)`
in the given `fs`, checks whether the given directory inode `inode` contains an `ext2_directory_entry` with name `name`. if so, returns `true` and sets `*out` to point to the entry's inode. otherwise returns `false` and sets `*out` to point to `INODE_ERROR`.

### `bool dir_contains(struct fs_info *fs, uint32_t parent_inode_n, const char *name, uint32_t *out)`
checsk whether, in the given `fs`, an inode with the name `name` exists in the directory `parent_inode_n`. If anything goes wrong or it's not inside then return `false`, otherwise return `true`

### `static bool dir_is_empty(struct fs_info *fs, struct ext2_inode inode)`
like `name_in()`, but instead scans to see whether it has any `ext2_directory_entry` besides `.` and `..`

### `bool dir_insert(struct fs_info *fs, uint32_t parent_inode_n, const char *name, uint32_t child_inode_n)`
does numerous checks to see whether the given `parent_inode_n` and `child_inode_n` are valid for insertion, then attempts to find a slot in the parent inode which fits the new entry that will be created. if it finds one, it attempts to add that inode to that directory, returning whether it succeeded or not. otherwise, it just returns `false`.

### `bool dir_remove(struct fs_info *fs, uint32_t parent_inode_n, const char *name, uint32_t *removed_inode_n)`
in the given `fs`, attempts to remove an inode with the given name `name` from the parent inode. then it looks through all the `ext2_directory_entry`s in the parent inode. if it ends up finding one with name `name`, then it attempts to remove it and returns the success (also sets `removed_inode_n` to the inode number of the removed entry). otherwise, it returns false as it didn't find any inode with that name.

### `uint32_t make_dir(struct fs_info *fs, uint32_t parent_inode_n, const char *name, struct fs_info **fs_out)`
attempts to make a directory inside the parent directory at `parent_inode_n` in the given `fs` and sets the new entry's name to `name` using the function `dir_insert()`. if `parent_inode_n` is a mountpoint the directory is made in the mounted fs's root instead. if it fails to insert (or the parent inode given is not valid), returns `INODE_ERROR`, if it succeeds, returns the new directory's inode number and puts the fs it was made in into `*fs_out`.

### `bool unlink_inode(struct fs_info *fs, uint32_t parent_inode_n, const char *name)`
unlinks the inode with name `name` inside the directory and the given `fs` with inode number `parent_inode_n`. checks if `parent_inode_n` is valid, if not returns `false`. then, after the validity checks, attempts to call `dir_remove()` with the given parameters, then calls  `delete_inode()` on the returned inode that was removed from the directory. 

### `bool unlink_dir(struct fs_info *fs, uint32_t parent_inode_n, const char *name)`
like `unlink_inode()` but for directories. refuses `.` and `..`, refuses a mountpoint, refuses anything that isn't a directory and refuses a directory that isn't empty (via `dir_is_empty()`). then calls `dir_remove()`, decrements the parent's `hard_link_cnt` and calls `delete_inode()` on the removed directory.

### `uint32_t make_file(struct fs_info *fs, uint32_t parent_inode_n, const char *name, struct fs_info **fs_out)`
like `make_dir()` but makes an empty regular file with `put_inode()`. returns the new inode number (or `INODE_ERROR`) and puts the fs it was made in into `*fs_out`. if `dir_insert()` fails the inode is freed again.

### `static bool dir_set_parent_entry(struct fs_info *fs, uint32_t child_n, uint32_t new_parent_n)`
rewrites the `..` entry of directory `child_n` to point at `new_parent_n`. both have to be directories. walks every block of the child looking for the `..` entry and returns `false` if it never finds it or an entry looks corrupt. used by `rename_inode()` when a directory moves.

### `static bool is_ancestor(struct fs_info *fs, uint32_t maybe_ancestor, uint32_t maybe_descendant)`
follows `..` upwards from `maybe_descendant` (at most `MAX_DIR_DEPTH` hops) and returns `true` if it hits `maybe_ancestor` before reaching `ROOT_INODE_N`. any failure along the way also returns `true`, so the caller errs on the side of refusing the move.

### `bool rename_inode(struct fs_info *fs, uint32_t old_parent_n, const char *old_name, uint32_t new_parent_n, const char *new_name)`
moves the entry `old_name` in `old_parent_n` to `new_name` in `new_parent_n`. refuses `.` and `..` as either name, refuses when the two parents end up in different filesystems, refuses a mountpoint, refuses the root directory, refuses a taken `new_name` and refuses moving a directory into its own descendant (`is_ancestor()`). inserts into the new parent first, then removes from the old one. if the child is a directory its `..` is rewritten with `dir_set_parent_entry()` and the two parents' `hard_link_cnt`s are adjusted.

### `bool make_symlink(struct fs_info *fs, uint32_t parent_n, const char *name, const char *target)`
makes a symlink named `name` in `parent_n` pointing at `target`. `target` has to be non empty and shorter than `BLOCK_SIZE`. if it fits in `MAX_SYMLINK_LEN` it is stored inside the inode's block pointer table, otherwise it gets a real block. then inserts it into the parent with `dir_insert()`.

### `static bool ls_dir(uint16_t *buf, struct dir_row *out, uint32_t *len, uint32_t size, uint32_t max)`
walks the directory block(s) in `buf` (`size` bytes) and fills `out` with one `dir_row` per live entry, writing the count into `*len`. returns `false` if it runs out of room (`max` rows) or hits a zero-sized entry. currently has no callers.
