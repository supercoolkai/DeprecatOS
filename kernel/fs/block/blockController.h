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

#define PARTS_PER_DRIVE 4

#define LOOKUP_MODE 0
#define ALLOC_MODE 1
#define FREE_MODE 2

#define MAX_MOUNTS 8
#define FS_INFO_LABEL_LEN 17

struct fs_info {
  uint16_t bgdt_buf[WORDS_PER_BLK];                        
  uint16_t sprblk_buf[sizeof(struct ext2_superblock) / 2]; 
  struct ata_drive *drive;
  struct ext2_superblock *superblk;
  struct ext2_block_group_descriptor *bgdt;

  uint64_t start_lba;
  uint64_t sector_count;

  uint32_t inode_size;       
  uint32_t inodes_per_grp;   
  uint32_t blks_per_grp;     
  uint32_t frags_per_grp;    
  uint32_t ngroups;          
  
  uint32_t mountpoint;
  char label[FS_INFO_LABEL_LEN];
};

extern struct fs_info fs_entries[MAX_MOUNTS];
extern struct fs_info *root_fs;

void read_block(struct fs_info *fs, uint32_t block_n, uint16_t *buf);
void write_block(struct fs_info *fs, uint32_t block_n, uint16_t *buf);
void block_init(void);
bool get_inode(struct fs_info *fs, uint32_t inode_n, struct ext2_inode *out);
uint32_t put_inode(struct fs_info *fs, uint16_t *buf, uint32_t f_size, bool is_dir, struct ext2_inode *out);
bool set_inode(struct fs_info *fs, uint32_t inode_n, struct ext2_inode *in);
bool delete_inode(struct fs_info *fs, uint32_t inode_n);
uint32_t write_inode(struct fs_info *fs, uint16_t *buf, struct ext2_inode *inode);
uint32_t replace_inode(struct fs_info *fs, uint32_t inode_n, uint16_t *buf, uint32_t f_size, struct ext2_inode *out);
uint32_t append_to_inode(struct fs_info *fs, uint32_t inode_n, uint16_t *buf, uint32_t f_size, struct ext2_inode *out);
uint32_t insert_in_inode(struct fs_info *fs, uint32_t inode_n, uint16_t *buf, uint32_t f_size, uint32_t offset, struct ext2_inode *out);
uint32_t mint_inode(struct fs_info *fs, uint32_t f_size, uint32_t type, struct ext2_inode *out);
uint32_t map_file_block(struct fs_info *fs, struct ext2_inode *inode, uint32_t file_blk_idx, uint32_t mode, uint32_t *disk_blk_out, uint32_t *alloc_cnt);
void unwind_insert(struct fs_info *fs, uint32_t inode_n, struct ext2_inode *inode, uint32_t *unwind_buf, uint32_t unwind_cursor, uint32_t free_cnt, uint32_t minted_cnt);
uint32_t truncate_inode(struct fs_info *fs, uint32_t inode_n, uint32_t new_size, struct ext2_inode *out);

#endif
