# Bitmap Controller

## global variables

#### `static struct ext2_superblock *superblk`:
a pointer to the main superblock, contains main information about the filesystem like the `bgdt`.

#### `static uint16_t bgdt_buf[WORDS_PER_BLK]`:
a buffer for filling out `bgdt`, used in `ext2_bitmap_init()`

#### `static uint16_t superblk_buf[sizeof(struct ext2_superblock) / 2]`:
a buffer for filling out `superblk`, used in `ext2_bitmap_init()`

#### `static struct ext2_block_group_descriptor *bgdt`:
a table of all the `ext2_block_group_descriptor`s in the filesystem.

#### `static uint32_t superblk_pos`:
the position of the superblock

#### `static uint32_t blks_per_grp`:
the number of blocks per group

#### `static uint32_t inodes_per_grp`:
the number of inodes per group

#### `static uint32_t first_block_n`:
the block number of the first block (`superblk`)

#### `static uint32_t ngroups`:
the number of total `ext2_block_group_descriptor`s in the `bgdt`

#### `static uint32_t bgdt_blk_n`:
the block number of the `bgdt`

#### `static uint32_t first_inode_n`:
the first valid inode number, like `first_block_n`

## overview
a dedicated allocator for the block/inode bitmap. currently supports freeing and allocating entries in both the inode_bitmap and the block_bitmap

## function analysis

### `void set_bitmap_controller_bgdt(struct ext2_block_group_descriptor *new_bgdt)`
sets `bgdt` to `new_bgdt` in order to maintain sync between all files using `bgdt`

### `void set_bitmap_controller_superblk(struct ext2_superblock *new_superblk)`
sets `superblk` to `new_superblk` in order to maintain sync between all files using `superblk`

### `void ext2_bitmap_init(void)`
the init function of the file, much like `block_init()` in the block controller. 

### `uint32_t alloc_inode(void)`
allocates a bit in the inode bitmap, and returns the inode number of the entry. decrements the `superblk`'s `unalloc_inodes` count as well as the `bgdt[group_n]`'s `unalloc_inode_cnt`


### `uint32_t alloc_block(void)`
like `alloc_inode()` but for blocks instead of inodes. allocates a bit in the block bitmap, and then returns the block number of the entry. decrements the `superblk`'s `unalloc_blocks` count as well as the `bgdt[group_n]`'s `unalloc_block_cnt`

### `bool free_inode(uint32_t inode_n)`
frees the given `inode_n`'s bit from the inode bitmap and increments the `superblk`'s `unalloc_inodes` count as well as the `bgdt[group_n]`'s `unalloc_inode_cnt`. returns a `bool` based off of whether it succeeded or not 

### `bool free_block(uint32_t block_n)`
frees the given `block_n`'s bit from the block bitmap and increments the `superblk`'s `unalloc_blocks` count as well as the `bgdt[group_n]`'s `unalloc_block_cnt`. returns a `bool` based off of whether or not it succeeded or not.
