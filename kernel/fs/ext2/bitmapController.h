#ifndef BITMAPCONTROLLER_H
#define BITMAPCONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "fs/ext2/blockGroupDescriptor.h"
#include "fs/ext2/superblock.h"

void ext2_bitmap_init(void);
uint32_t alloc_inode(void);
uint32_t alloc_block(void);
bool free_inode(uint32_t inode_n);
bool free_block(uint32_t block_n);
void set_bitmap_controller_bgdt(struct ext2_block_group_descriptor *new_bgdt);
void set_bitmap_controller_superblk(struct ext2_superblock *new_superblk);

#endif
