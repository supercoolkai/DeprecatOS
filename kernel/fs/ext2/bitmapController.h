#ifndef BITMAPCONTROLLER_H
#define BITMAPCONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "fs/ext2/blockGroupDescriptor.h"
#include "fs/ext2/superblock.h"

uint32_t alloc_inode(void);
uint32_t alloc_block(void);
bool free_inode(uint32_t inode_n);
bool free_block(uint32_t block_n);
bool inode_exists(uint32_t inode_n);

#endif
