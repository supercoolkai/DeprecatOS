# Block Controller

## global variables
#### `static struct ext2_superblock *superblk`:
a pointer to the superblock (bytes 1024 -> 2047 of block 0)

#### `static uint16_t sprblk_buf[sizeof(struct ext2_superblock) / 2]`:
a temporary buffer for setting superblk

#### `static uint16_t bgdt_buf[WORDS_PER_BLK]`:
a temporary buffer for setting bgdt

#### `static struct ext2_block_group_descriptor *bgdt`:
a pointer to the block group descriptor table (bytes 0 -> 1023 of block 1)

#### `static uint16_t sectors_per_blk`:
how many sectors there are per block

#### `static uint32_t superblk_pos`:
the position of the superblock in block 0

#### `static uint32_t bgdt_blk_n`:
which block number the bgdt is in

#### `static uint16_t buf[WORDS_PER_BLK]`:
a buffer used in `get_inode()` to read a block in order to retrieve an inode

#### `static uint32_t inode_size`:
the superblock's `inode_size` field

#### `static uint32_t inodes_per_grp`:
the superblock's `inodes_per_group` field

#### `static uint32_t blks_per_grp`:
the superblock's `blocks_per_group` field

#### `static uint32_t frags_per_grp`:
the superblock's `frags_per_group` field

#### `static uint32_t ngroups`:
the number of groups there are going to be in the bgdt

#### `static uint16_t indirect_buf[3][WORDS_PER_BLK]`:
like `buf`, but for the `indir_read_block()` function instead

#### `statc uint16_t *out_cursor`:
a variable used in `indir_read_block()` to advance where the block being read will be deposited into on `indirect_buf`

#### `static uint32_t blocks_left`:
a variable used in `indir_read_block()` and set in `read_inode()` to know how many blocks there are left to read


## overview
controller of the blocks in the filesystem, currently only supports read functions. has `read_block()` which is non-recursive and `indir_read_block()` which is recursive. also gets/retrieves inodes through the `get/read_inode()` functions.

## function analysis

### `void read_block(uint32_t block_n, uint16_t *buf)`
reads a full block into `buf` using `ata_read48()` from the ata driver in `src/drivers/disk/ata.h`

### `static void indir_read_block(uint32_t block_n, int layer)`
the recursive version of `read_block()`, used to read inodes which are larger than 48 KiB large. compatible with inodes anywhere under 4 TiB

### `void block_init(void)`
sets up numerous global variables alongside the `superblk` and `bgdt` variables.

### `bool get_inode(uint32_t inode_n, struct ext2_inode *out)`
gets the real `ext2_inode` struct from the unique `inode_n` representing it. returns a bool based off of success in this operation.

### `void read_inode(struct ext2_inode *inode, uint16_t *out)`
calls the `indir_read_block()` function to read an inode typically returned from `get_inode()` into the buffer `out`. do note that it completely trusts in `indir_read_block()` to work properly and does not return/warn anything if something goes wrong, only panics if the block is out of range.
