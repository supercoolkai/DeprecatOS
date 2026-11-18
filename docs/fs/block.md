# Block Controller

## fs_info struct format
src `kernel/fs/block/blockController.h`

a struct which describes an ext2 filesystem (basically like `ata_drive`, but for valid partitions rather than entire drives)

| offset | field | type |
| --- | --- | --- |
| 0 | `bgdt_buf` | `uint16_t[WORDS_PER_BLK]` |
| 1 | `sprblk_buf` | `uint16_t[sizeof(struct ext2_superblock) / 2]` |
| 2 | `drive` | `struct ata_drive *` |
| 3 | `superblk` | `struct ext2_superblock *` |
| 4 | `bgdt` | `struct ext2_block_group_descriptor *` |
| 5 |`start_lba` | `uint64_t` |
| 6 | `sector_count` | `uint64_t` |
| 7 | `inode_size` | `uint32_t` |
| 8 | `inodes_per_grp` | `uint32_t` |
| 9 | `blks_per_grp` | `uint32_t` |
| 10 | `frags_per_grp` | `uint32_t` |
| 11 | `ngroups` | `uint32_t` |
| 12 | `mountpoint` | `uint32_t` |
| 13 | `label` | `char[FS_INFO_LABEL_LEN]` |

--- 

## global variables
#### `static uint16_t sectors_per_blk`:
how many sectors there are per block

#### `static uint32_t superblk_pos`:
the position of the superblock in block 0

#### `static uint32_t bgdt_blk_n`:
which block number the bgdt is in

#### `static uint32_t first_inode_n`:
the first valid inode number, used to turn an inode number into its group number

#### `struct fs_info fs_entries[MAX_MOUNTS]`:
the table of every partition found on the master and slave drives, filled by `fill_partition_entry()` in `block_init()`. `PARTS_PER_DRIVE` slots per drive, an entry with `sector_count` of 0 is unused

#### `struct fs_info *root_fs`:
a pointer into `fs_entries` at the partition labeled `deprecatOS`, panics in `block_init()` if none is found


## overview
controller of the blocks in the filesystem, every function takes the `fs_info` of the partition it's working on. has `read_block()` which is non-recursive and `indir_read_block()` which is recursive. also gets/retrieves inodes through the `get/read_inode()` functions. supports `write_inode()` and `delete_inode()`, which use the `write_block()` and `indir_free_block()` functions, plus the in-place edits `replace_inode()`, `append_to_inode()`, `insert_in_inode()` and `truncate_inode()`. also finds every partition on both drives in `block_init()` and picks `root_fs` out of them.

## function analysis

### `void read_block(struct fs_info *fs, uint32_t block_n, uint16_t *buf)`
reads a full block to `buf` using `ata_read48_part()` from the ata driver in `kernel/drivers/disk/ata.h`. uses the drive from the given `fs`

### `void write_block(struct fs_info *fs, uint32_t block_n, uint16_t *buf)`
writes a full block from `buf` using `ata_write48_part()` from the ata driver in `kernel/drivers/disk/ata.h`. uses the drive from the given `fs`

### `static void indir_read_block(struct fs_info *fs, uint16_t **out_cursor, uint32_t *blocks_left, uint32_t block_n, int layer, uint16_t (*indirect_buf)[WORDS_PER_BLK])`
the recursive version of `read_block()`, used to read inodes which are larger than 48 KiB large. compatible with inodes anywhere under 4 TiB

### `static void indir_free_block(struct fs_info *fs, uint32_t block_n, uint32_t layer, uint32_t (*indirect_buf)[BIT_32_PER_BLK])`
the recursive version of the bitmap function `free_block()`, used to free inodes which are larger than 48 KiB large. compatible with inodes anywhere under 4 TiB

### `static bool fill_partition_entry(uint8_t drive_magic, uint32_t part)`
fills an `fs_info` entry and puts it into the array `fs_entries` at index `part`

### `void block_init(void)`
sets up the array `fs_entries` and also the `root_fs` variable

### `bool get_inode(struct fs_info *fs, uint32_t inode_n, struct ext2_inode *out)`
gets the real `ext2_inode` struct from the unique `inode_n` representing it and the `fs` it's in. returns a bool based off of success in this operation.

### `bool set_inode(struct fs_info *fs, uint32_t inode_n, struct ext2_inode *in)`
like `put_inode()`, but instead it redirects the given `inode_n` to point to the given inode `in`

### `static void zero_block_buf(uint32_t *buf)`
zeroes the given array at `buf`. must be exactly `BIT_32_PER_BLK` in size, because it must be some form of a block buffer.

### `bool delete_inode(struct fs_info *fs, uint32_t inode_n)`
frees all the blocks at the inode at `inode_n` and filesystem `fs` and saves it to disk. do note that this does not unlink it, it only deletes the data inside of it.

### `uint32_t mint_inode(struct fs_info *fs, uint32_t f_size, uint32_t type, struct ext2_inode *out)`
creates a blank inode in filesystem `fs` with its `size_lo` being `f_size`, its `type_and_perms_lo` being `type`, and sets up all other relevant fields. then, sets `*out` to point to the new inode and returns its number.

### `uint32_t write_inode(struct fs_info *fs, uint16_t *buf, struct ext2_inode *inode)`
a function similar to `indir_read_block()` but it writes the data blocks of an already minted inode from scratch, with data `buf` into filesystem `fs`. iterative because too complicated to do recursive (maybe i was too lazy idk. either way its fine). zeroes all of `inode`'s block pointers then allocates and fills blocks until `inode->size_lo` bytes are written, so `size_lo` MUST match `buf`'s size. returns `INODE_WRITE_SUCCESS`, or `BLOCK_ERROR` after freeing what it allocated if a block allocation fails. called by `put_inode()` after `mint_inode()`.

### `uint32_t replace_inode(struct fs_info *fs, uint32_t inode_n, uint16_t *buf, uint32_t f_size, struct ext2_inode *out)`
replaces the given `fs`'s `inode_n`'s contents with the contents inside `buf`

### `uint32_t map_file_block(struct fs_info *fs, struct ext2_inode *inode, uint32_t file_blk_idx, uint32_t mode, uint32_t *disk_blk_out, uint32_t *alloc_cnt)`
a really really naive function (which probably should be iterative), which looks up the given `file_blk_idx` in the inode `inode` in filesystem `fs`. it essentially searches the inode's block pointers for it, and if it doesnt exist, two things can happen: if `mode` is `LOOKUP_MODE` or `FREE_MODE`, then it returns `EMPTY_BLOCK`. if `mode` is `ALLOC_MODE`, it allocates a new block and points the block pointer to it, increments `alloc_cnt` if it exists, then continues on with the function. if `mode` is `FREE_MODE` then it unlinks the block pointer.

### `uint32_t append_to_inode(struct fs_info *fs, uint32_t inode_n, uint16_t *buf, uint32_t f_size, struct ext2_inode *out)`
appends the given `buf` to the end of the inode at `inode_n` and filesystem `fs`, and updates relevant inode fields (`last_mod_time`, `last_access_time`, `disk_sectors`, `size_lo`). afterwards it returns the inode at `inode_n`

### `void unwind_insert(struct fs_info *fs, uint32_t inode_n, struct ext2_inode *inode, uint32_t *unwind_buf, uint32_t unwind_cursor, uint32_t free_cnt, uint32_t minted_cnt)`
a free function for `insert_in_inode()`, first frees all data blocks then frees all pointer blocks which were created in the insert.

### `uint32_t insert_in_inode(struct fs_info *fs, uint32_t inode_n, uint16_t *buf, uint32_t f_size, uint32_t offset, struct ext2_inode *out)`
like `append_to_inode()`, but it allows you to insert data in an arbitrary location. calls `unwind_insert()` upon failure and returns the inode at `inode_n`, also updates relevant inode fields (`last_mod_time`, `last_access_time`, `disk_sectors`, `size_lo`)

### `uint32_t put_inode(struct fs_info *fs, uint16_t *buf, uint32_t f_size, bool is_dir, struct ext2_inode *out)`
a wrapper of `write_inode()` which wires it into the `bgdt`.

### `uint32_t truncate_inode(struct fs_info *fs, uint32_t inode_n, uint32_t new_size, struct ext2_inode *out)`
sets the given `fs`'s `inode_n`'s size to `new_size`. cannot increase size 
