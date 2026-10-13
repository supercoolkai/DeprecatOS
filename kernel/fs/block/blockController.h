#ifndef BLOCKCONTROLLER_H
#define BLOCKCONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "fs/ext2/inode.h"
#include "fs/ext2/blockGroupDescriptor.h"
#include "fs/ext2/superblock.h"
#include "drivers/disk/ata.h"

#define WORDS_PER_BLK (BLOCK_SIZE / 2)
#define BIT_32_PER_BLK (BLOCK_SIZE / 4)
#define EXT2_SUPERBLOCK_OFFSET 1024
#define INDIRECT_PTR_LAYERS 3

#define LOOKUP_MODE 0
#define ALLOC_MODE 1
#define FREE_MODE 2

void read_block(uint32_t block_n, uint16_t *buf);
void write_block(uint32_t block_n, uint16_t *buf);
void block_init(void);
bool get_inode(uint32_t inode_n, struct ext2_inode *out);
uint32_t put_inode(uint16_t *buf, uint32_t f_size, bool is_dir, struct ext2_inode *out);
void set_block_controller_bgdt(struct ext2_block_group_descriptor *new_bgdt);
void set_block_controller_superblk(struct ext2_superblock *new_superblk);
bool set_inode(uint32_t inode_n, struct ext2_inode *in);
bool delete_inode(uint32_t inode_n);
uint32_t write_inode(uint16_t *buf, struct ext2_inode *inode);
uint32_t replace_inode(uint32_t inode_n, uint16_t *buf, uint32_t f_size, struct ext2_inode *out);
uint32_t append_to_inode(uint32_t inode_n, uint16_t *buf, uint32_t f_size, struct ext2_inode *out);
uint32_t insert_in_inode(uint32_t inode_n, uint16_t *buf, uint32_t f_size, uint32_t offset, struct ext2_inode *out);
uint32_t mint_inode(uint32_t f_size, uint32_t type, struct ext2_inode *out);
uint32_t map_file_block(struct ext2_inode *inode, uint32_t file_blk_idx, uint32_t mode, uint32_t *disk_blk_out, uint32_t *alloc_cnt);
void unwind_insert(uint32_t inode_n, struct ext2_inode *inode, uint32_t *unwind_buf, uint32_t unwind_cursor, uint32_t free_cnt, uint32_t minted_cnt);
uint32_t truncate_inode(uint32_t inode_n, uint32_t new_size, struct ext2_inode *out);

#endif
