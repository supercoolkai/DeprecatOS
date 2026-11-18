# Bitmap Controller

## global variables
#### `static uint32_t superblk_pos`:
the position of the superblock

#### `static uint32_t bgdt_blk_n`:
the block number of the `bgdt`

#### `static uint32_t first_inode_n`:
the first valid inode number, like `first_block_n`

## overview
a dedicated allocator for the block/inode bitmap. currently supports freeing and allocating entries in both the inode_bitmap and the block_bitmap

## function analysis

### `uint32_t alloc_inode(struct fs_info *fs)`
allocates a bit in the `fs`'s inode bitmap, and returns the inode number of the entry. decrements the `superblk`'s `unalloc_inodes` count as well as the `bgdt[group_n]`'s `unalloc_inode_cnt`


### `uint32_t alloc_block(void)`
like `alloc_inode()` but for blocks instead of inodes. allocates a bit in the `fs`'s block bitmap, and then returns the block number of the entry. decrements the `superblk`'s `unalloc_blocks` count as well as the `bgdt[group_n]`'s `unalloc_block_cnt`

### `bool free_inode(struct fs_info *fs, uint32_t inode_n)`
frees the given `inode_n`'s bit from the `fs`'s inode bitmap and increments the `superblk`'s `unalloc_inodes` count as well as the `bgdt[group_n]`'s `unalloc_inode_cnt`. returns a `bool` based off of whether it succeeded or not 

### `bool free_block(struct fs_info *fs, uint32_t block_n)`
frees the given `block_n`'s bit from the `fs`'s block bitmap and increments the `superblk`'s `unalloc_blocks` count as well as the `bgdt[group_n]`'s `unalloc_block_cnt`. returns a `bool` based off of whether or not it succeeded or not.

### `bool inode_exists(struct fs_info *fs, uint32_t inode_n)`
returns the `fs`'s corresponding inode bitmap value for the given `inode_n`
