#include "fs/ext2/bitmapController.h" 
#include "fs/ext2/blockGroupDescriptor.h"
#include "fs/ext2/inode.h"
#include "fs/ext2/superblock.h"
#include "fs/block/blockController.h"
#include "util/kprintf/kprintf.h"
#include "exceptions/exceptions.h"
#include "memory/heap/kernelHeap.h"
#include "errors.h"
#include <stdint.h>
#include <stdbool.h>

static struct ext2_superblock *superblk;
static uint16_t bgdt_buf[WORDS_PER_BLK];
static uint16_t sprblk_buf[sizeof(struct ext2_superblock) / 2];
static struct ext2_block_group_descriptor *bgdt;
static uint32_t superblk_pos = EXT2_SUPERBLOCK_OFFSET;

static uint32_t blks_per_grp;
static uint32_t inodes_per_grp;
static uint32_t first_block_n;
static uint32_t ngroups;
static uint32_t bgdt_blk_n = 1;
static uint32_t first_inode_n = 1;

// a quick connection point between
// different files using the bgdt,
// put this in all files using bgdt
void set_bitmap_controller_bgdt(struct ext2_block_group_descriptor *new_bgdt){
  bgdt = new_bgdt;
}

// a quick connection point between
// different files using the superblock,
// put this in all files using the superblock
void set_bitmap_controller_superblk(struct ext2_superblock *new_superblk){
  superblk = new_superblk;
}

void ext2_bitmap_init(void)
{
  ata_read48(ATA_MASTER, superblk_pos / BYTES_PER_SECTOR, sizeof(struct ext2_superblock) / BYTES_PER_SECTOR, sprblk_buf);
  read_block(bgdt_blk_n, bgdt_buf);

  superblk = (struct ext2_superblock *) sprblk_buf;
  bgdt = (struct ext2_block_group_descriptor *) bgdt_buf;
  
  blks_per_grp = superblk->blocks_per_group;
  first_block_n = superblk->super_block_num;
  inodes_per_grp = superblk->inodes_per_group;
  ngroups = (superblk->block_cnt + blks_per_grp - 1) / blks_per_grp;
}

uint32_t alloc_inode(void)
{
  uint32_t group_n;
  bool found_grp = false;

  for(group_n = 0; group_n < ngroups; group_n++){
    if (bgdt[group_n].unalloc_inode_cnt > 0) {
      found_grp = true;
      break;
    }
  }

  if (!found_grp) {
    kprintf(KPRINTF_RED "Tried to allocate inode but no inodes left");
    return INODE_ERROR;
  }

  uint16_t *bitmap_buf = (uint16_t *) kmalloc(BLOCK_SIZE);
  read_block(bgdt[group_n].inode_bitmap_addr, bitmap_buf);

  uint8_t *bitmap = (uint8_t *)bitmap_buf;

  uint32_t grp_inode_idx = 0;
  bool found_inode = false;
  for (uint32_t byte_idx = 0; byte_idx < inodes_per_grp / 8; byte_idx++)
  {
    for (uint8_t bit = 0; bit < 8; bit++) {
      uint8_t mask = 1 << bit;
      bool free = !(bitmap[byte_idx] & mask);

      if (free) {
        found_inode = true;
        bitmap[byte_idx] |= mask;
        bgdt[group_n].unalloc_inode_cnt--;
        superblk->unalloc_inodes--;

        write_block(bgdt_blk_n, (uint16_t *) bgdt);
        ata_write48(ATA_MASTER, superblk_pos / BYTES_PER_SECTOR, sizeof(struct ext2_superblock) / BYTES_PER_SECTOR, sprblk_buf);
        write_block(bgdt[group_n].inode_bitmap_addr, (uint16_t *) bitmap);
        set_block_controller_bgdt(bgdt);
        set_block_controller_superblk(superblk);
        break;
      }

      grp_inode_idx++;
    }

    if (found_inode)
      break;
  }

  if (!found_inode) {
    panic("KERNEL PANIC: CORRUPTED UNALLOC_INODE_CNT!!!");
  }

  kfree(bitmap_buf);

  return group_n * inodes_per_grp + grp_inode_idx + first_inode_n;
}

uint32_t alloc_block(void)
{
  uint32_t group_n;
  bool found_grp = false;

  for(group_n = 0; group_n < ngroups; group_n++){
    if (bgdt[group_n].unalloc_block_cnt > 0) {
      found_grp = true;
      break;
    }
  }

  if (!found_grp) {
    kprintf(KPRINTF_RED "Tried to allocate block but no blocks left");
    return BLOCK_ERROR;
  }

  uint16_t *bitmap_buf = (uint16_t *) kmalloc(BLOCK_SIZE);
  read_block(bgdt[group_n].block_bitmap_addr, bitmap_buf);

  uint8_t *bitmap = (uint8_t *)bitmap_buf;

  uint32_t grp_block_idx = 0;
  bool found_blk = false;
  for (uint32_t byte_idx = 0; byte_idx < BLOCK_SIZE; byte_idx++)
  {
    for (uint8_t bit = 0; bit < 8; bit++) {
      uint8_t mask = 1 << bit;
      bool free = !(bitmap[byte_idx] & mask);

      if (free) {
        found_blk = true;
        bitmap[byte_idx] |= mask;
        bgdt[group_n].unalloc_block_cnt--;
        superblk->unalloc_blocks--;

        write_block(bgdt_blk_n, (uint16_t *) bgdt);
        ata_write48(ATA_MASTER, superblk_pos / BYTES_PER_SECTOR, sizeof(struct ext2_superblock) / BYTES_PER_SECTOR, sprblk_buf);
        write_block(bgdt[group_n].block_bitmap_addr, (uint16_t *) bitmap);
        set_block_controller_bgdt(bgdt);
        set_block_controller_superblk(superblk);
        break;
      }

      grp_block_idx++;
    }

    if (found_blk)
      break;
  }

  if (!found_blk) {
    panic("KERNEL PANIC: CORRUPTED UNALLOC_BLOCK_CNT!!!");
  }

  kfree(bitmap_buf);

  return group_n * blks_per_grp + grp_block_idx + first_block_n;
}

bool free_inode(uint32_t inode_n)
{
  uint32_t group_n = (inode_n - first_inode_n) / inodes_per_grp;
  
  if (group_n >= ngroups || inode_n > superblk->inode_cnt) {
    kprintf(KPRINTF_RED "Invalid inode number");
    return false;
  }

  uint32_t bitmap_idx = (inode_n - first_inode_n) % inodes_per_grp;

  uint16_t *bitmap_buf = (uint16_t *) kmalloc(BLOCK_SIZE);
  read_block(bgdt[group_n].inode_bitmap_addr, bitmap_buf);

  uint8_t *bitmap = (uint8_t *)bitmap_buf;

  uint32_t byte_idx = bitmap_idx / 8;
  uint8_t mask = 1 << (bitmap_idx % 8);
  
  bool free_alr = !(bitmap[byte_idx] & mask);

  if (!free_alr){
    bitmap[byte_idx] &= ~mask;
    // for alloc its bitmap[byte] |= mask
  
    bgdt[group_n].unalloc_inode_cnt++;
    superblk->unalloc_inodes++;

    write_block(bgdt_blk_n, (uint16_t *) bgdt);
    ata_write48(ATA_MASTER, superblk_pos / BYTES_PER_SECTOR, sizeof(struct ext2_superblock) / BYTES_PER_SECTOR, sprblk_buf);
    write_block(bgdt[group_n].inode_bitmap_addr, (uint16_t *) bitmap);
    set_block_controller_bgdt(bgdt);
    set_block_controller_superblk(superblk);
  }

  kfree(bitmap_buf);
  return true;
}

bool free_block(uint32_t block_n)
{
  uint32_t group_n = (block_n - first_block_n) / blks_per_grp;
  
  if (group_n >= ngroups || block_n >= superblk->block_cnt) {
    kprintf(KPRINTF_RED "Invalid block number");
    return false;
  }

  uint32_t bitmap_idx = (block_n - first_block_n) % blks_per_grp;

  uint16_t *bitmap_buf = (uint16_t *) kmalloc(BLOCK_SIZE);
  read_block(bgdt[group_n].block_bitmap_addr, bitmap_buf);

  uint8_t *bitmap = (uint8_t *)bitmap_buf;

  uint32_t byte_idx = bitmap_idx / 8;
  uint8_t mask = 1 << (bitmap_idx % 8);
  
  bool free_alr = !(bitmap[byte_idx] & mask);

  if (!free_alr){
    bitmap[byte_idx] &= ~mask;
    // for alloc its bitmap[byte] |= mask
  
    bgdt[group_n].unalloc_block_cnt++;
    superblk->unalloc_blocks++;

    write_block(bgdt_blk_n, (uint16_t *) bgdt);
    ata_write48(ATA_MASTER, superblk_pos / BYTES_PER_SECTOR, sizeof(struct ext2_superblock) / BYTES_PER_SECTOR, sprblk_buf);
    write_block(bgdt[group_n].block_bitmap_addr, (uint16_t *) bitmap);
    set_block_controller_bgdt(bgdt);
    set_block_controller_superblk(superblk);
  }

  kfree(bitmap_buf);
  return true;
}

