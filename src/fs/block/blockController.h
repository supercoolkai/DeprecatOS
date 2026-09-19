#ifndef BLOCKCONTROLLER_H
#define BLOCKCONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "fs/ext2/inode.h"
#include "fs/ext2/blockGroupDescriptor.h"
#include "drivers/disk/ata.h"

#define WORDS_PER_BLK (BLOCK_SIZE / 2)
#define BIT_32_PER_BLK (BLOCK_SIZE / 4)
#define EXT2_SUPERBLOCK_OFFSET 1024
#define INDIRECT_PTR_LAYERS 3

void read_block(uint32_t block_n, uint16_t *buf);
void write_block(uint32_t block_n, uint16_t *buf);
void block_init(void);
bool get_inode(uint32_t inode_n, struct ext2_inode *out);
void read_inode(struct ext2_inode *inode, uint16_t *out);
uint32_t write_inode(uint16_t *buf, uint32_t f_size, bool is_dir, struct ext2_inode *out);

#endif
