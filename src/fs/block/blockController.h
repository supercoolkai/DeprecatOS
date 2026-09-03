#ifndef BLOCKCONTROLLER_H
#define BLOCKCONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "fs/ext2/inode.h"
#include "drivers/disk/ata.h"

#define WORDS_PER_BLK (BLOCK_SIZE / 2)
#define BIT_32_PER_BLK (BLOCK_SIZE / 4)

void read_block(uint32_t block_n, uint16_t *buf);
void block_init(void);
bool get_inode(uint32_t inode_n, struct ext2_inode *out);
void read_inode(struct ext2_inode *inode, uint16_t *out);

#endif
