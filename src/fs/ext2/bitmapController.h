#ifndef BITMAPCONTROLLER_H
#define BITMAPCONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

void ext2_bitmap_init(void);
uint32_t alloc_inode(void);
uint32_t alloc_block(void);
bool free_inode(uint32_t inode_n);
bool free_block(uint32_t block_n);

#endif
