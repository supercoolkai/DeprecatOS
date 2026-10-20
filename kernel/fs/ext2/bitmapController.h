#ifndef BITMAPCONTROLLER_H
#define BITMAPCONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

struct fs_info;
#include "fs/ext2/blockGroupDescriptor.h"
#include "fs/ext2/superblock.h"

uint32_t alloc_inode(struct fs_info *fs);
uint32_t alloc_block(struct fs_info *fs);
bool free_inode(struct fs_info *fs, uint32_t inode_n);
bool free_block(struct fs_info *fs, uint32_t block_n);
bool inode_exists(struct fs_info *fs, uint32_t inode_n);

#endif
