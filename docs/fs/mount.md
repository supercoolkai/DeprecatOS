# Mount module

## global variables
#### `static uint32_t root_slot_n`:
a variable showing the slot of the root filesystem
#### `static char mount_path[MOUNT_PATH_SEED_LEN + FS_INFO_LABEL_LEN]`:
a disposable variable used in `mount_fs` to find the mountpoint
#### `static uint32_t superblk_pos`:
basically an alias for `EXT2_SUPERBLOCK_OFFSET`

## overview
controller of mounts, basically the main driver between the operating system having partition awareness. every time u want to mount another filesystem you must call `mount` or `umount` here, also used in initialization of filesystems (`mark_fs_state()`)

## function analysis
### `void mark_fs_state(struct fs_info *fs, bool set_valid)`
sets or clears the `FS_STATE_VALID_BIT` in the given `fs`'s **superblock's** `fs_state` (based off of `set_valid`), then writes the superblock back to disk with `ata_write48_part()`. when clearing, if the bit was already clear it warns `partition was not cleanly unmounted` first

### `void mount_init(void)`
initializes the `root_slot_n` and calls `mark_fs_state()` on `root_fs` to make it `false`

### `bool return_mnt_root(struct fs_info *fs, uint32_t inode_n, struct fs_info **out, uint32_t *out_inode_n)`
returns the root inode of the given mountpoint (`inode_n`) in `fs`

### `bool return_mnt_mountpoint(struct fs_info *fs, uint32_t inode_n, struct fs_info **out, uint32_t *out_inode_n)`
returns the mountpoint of a given `fs` but ONLY if `inode_n == ROOT_INODE_N`

### `bool mount_fs(const char *label)`
attempts to find a filesystem with label `label`, and if it's not already mounted, mount it. the mountpoint is always `/mnt/<label>` (built from `MOUNT_PATH_SEED` into `mount_path`), which has to already exist, resolve inside `root_fs` and be a directory. rejects (with a yellow warning and `false`) a label that is `.` or `..`, a label containing `/`, a blank label, a duplicate of another slot's label, or a partition that is already mounted. on success sets the slot's `mountpoint` and calls `mark_fs_state()` with `false`

### `bool umount_fs(const char *label)`
attempts to find a filesystem with label `label`, and if it's not already unmounted, unmount it
