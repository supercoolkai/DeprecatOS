# Ext2 Filesystem

## structs
  - `ext2_block_group_descriptor`: contains general information, including the bitmap addresses, the address where the inodes start, the unallocated block count, the unallocated inode count, and the directory count.
  - `ext2_inode`: essentially a file, has a block pointers which point to different blocks containing the data that the inode contains. including indirect block pointers it can be up to 4 TiB per inode. each inode has a unique "id" (commonly referred to as inode_n)
  - `ext2_directory_entry`: a wrapper for the ext2_inode struct, containing an inode_n, the size of that inode, the length of its name (the hi byte can sometimes be the type as well), and its name. 
  - `ext2_superblock`: bytes 1023 - 2047 of block 0. contains general information about the filesystem, including inode count, block count, blocks per group, inodes per group, etc.

## controllers

### `directoryController`
as the name says, it is the main controller of directories on the kernel side. contains the tools for resolving paths, looking up dirs, etc. for more in-depth information, refer to `docs/fs/ext2/directoryController.md`
