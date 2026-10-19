#include "fs/ext2/bitmapController.h" 
#include "fs/ext2/blockGroupDescriptor.h"
#include "fs/ext2/inode.h"
#include "fs/ext2/superblock.h"
#include "fs/block/blockController.h"
#include "kprintf/kprintf.h"
#include "exceptions/exceptions.h"
#include "memory/heap/kernelHeap.h"
#include "errors.h"
#include <stdint.h>
#include <stdbool.h>

static uint32_t superblk_pos = EXT2_SUPERBLOCK_OFFSET;
static uint32_t bgdt_blk_n = 1;
static uint32_t first_inode_n = 1;

uint32_t alloc_inode(void)
{
  uint32_t group_n;
  bool found_grp = false;

  for(group_n = 0; group_n < root_fs->ngroups; group_n++){
    if (root_fs->bgdt[group_n].unalloc_inode_cnt > 0) {
      found_grp = true;
      break;
    }
  }

  if (!found_grp) {
    kprintf(KPRINTF_RED "Tried to allocate inode but no inodes left");
    return INODE_ERROR;
  }

  uint16_t *bitmap_buf = (uint16_t *) kmalloc(BLOCK_SIZE);
  read_block(root_fs->bgdt[group_n].inode_bitmap_addr, bitmap_buf);

  uint8_t *bitmap = (uint8_t *)bitmap_buf;

  uint32_t grp_inode_idx = 0;
  bool found_inode = false;
  for (uint32_t byte_idx = 0; byte_idx < root_fs->inodes_per_grp / 8; byte_idx++)
  {
    for (uint8_t bit = 0; bit < 8; bit++) {
      uint8_t mask = 1 << bit;
      bool free = !(bitmap[byte_idx] & mask);

      if (free) {
        found_inode = true;
        bitmap[byte_idx] |= mask;
        root_fs->bgdt[group_n].unalloc_inode_cnt--;
        root_fs->superblk->unalloc_inodes--;

        write_block(bgdt_blk_n, (uint16_t *) root_fs->bgdt);
        ata_write48_part(ATA_MASTER, root_fs->start_lba, root_fs->sector_count, superblk_pos / BYTES_PER_SECTOR, sizeof(struct ext2_superblock) / BYTES_PER_SECTOR, root_fs->sprblk_buf);
        write_block(root_fs->bgdt[group_n].inode_bitmap_addr, (uint16_t *) bitmap);
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

  return group_n * root_fs->inodes_per_grp + grp_inode_idx + first_inode_n;
}

uint32_t alloc_block(void)
{
  uint32_t group_n;
  bool found_grp = false;

  for(group_n = 0; group_n < root_fs->ngroups; group_n++){
    if (root_fs->bgdt[group_n].unalloc_block_cnt > 0) {
      found_grp = true;
      break;
    }
  }

  if (!found_grp) {
    kprintf(KPRINTF_RED "Tried to allocate block but no blocks left");
    return BLOCK_ERROR;
  }

  uint16_t *bitmap_buf = (uint16_t *) kmalloc(BLOCK_SIZE);
  read_block(root_fs->bgdt[group_n].block_bitmap_addr, bitmap_buf);

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
        root_fs->bgdt[group_n].unalloc_block_cnt--;
        root_fs->superblk->unalloc_blocks--;

        write_block(bgdt_blk_n, (uint16_t *) root_fs->bgdt);
        ata_write48_part(ATA_MASTER, root_fs->start_lba, root_fs->sector_count, superblk_pos / BYTES_PER_SECTOR, sizeof(struct ext2_superblock) / BYTES_PER_SECTOR, root_fs->sprblk_buf);
        write_block(root_fs->bgdt[group_n].block_bitmap_addr, (uint16_t *) bitmap);
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

  return group_n * root_fs->blks_per_grp + grp_block_idx + root_fs->superblk->super_block_num;
}

bool free_inode(uint32_t inode_n)
{
  uint32_t group_n = (inode_n - first_inode_n) / root_fs->inodes_per_grp;
  
  if (group_n >= root_fs->ngroups || inode_n > root_fs->superblk->inode_cnt) {
    kprintf(KPRINTF_RED "Invalid inode number");
    return false;
  }

  uint32_t bitmap_idx = (inode_n - first_inode_n) % root_fs->inodes_per_grp;

  uint16_t *bitmap_buf = (uint16_t *) kmalloc(BLOCK_SIZE);
  read_block(root_fs->bgdt[group_n].inode_bitmap_addr, bitmap_buf);

  uint8_t *bitmap = (uint8_t *)bitmap_buf;

  uint32_t byte_idx = bitmap_idx / 8;
  uint8_t mask = 1 << (bitmap_idx % 8);
  
  bool free_alr = !(bitmap[byte_idx] & mask);

  if (!free_alr){
    bitmap[byte_idx] &= ~mask;
    // for alloc its bitmap[byte] |= mask
  
    root_fs->bgdt[group_n].unalloc_inode_cnt++;
    root_fs->superblk->unalloc_inodes++;

    write_block(bgdt_blk_n, (uint16_t *) root_fs->bgdt);
    ata_write48_part(ATA_MASTER, root_fs->start_lba, root_fs->sector_count, superblk_pos / BYTES_PER_SECTOR, sizeof(struct ext2_superblock) / BYTES_PER_SECTOR, root_fs->sprblk_buf);
    write_block(root_fs->bgdt[group_n].inode_bitmap_addr, (uint16_t *) bitmap);
  }

  kfree(bitmap_buf);
  return true;
}

bool free_block(uint32_t block_n)
{
  if (block_n == 0)
    return false;

  uint32_t group_n = (block_n - root_fs->superblk->super_block_num) / root_fs->blks_per_grp;
  
  if (group_n >= root_fs->ngroups || block_n >= root_fs->superblk->block_cnt) {
    kprintf(KPRINTF_RED "Invalid block number");
    return false;
  }

  uint32_t bitmap_idx = (block_n - root_fs->superblk->super_block_num) % root_fs->blks_per_grp;

  uint16_t *bitmap_buf = (uint16_t *) kmalloc(BLOCK_SIZE);
  read_block(root_fs->bgdt[group_n].block_bitmap_addr, bitmap_buf);

  uint8_t *bitmap = (uint8_t *)bitmap_buf;

  uint32_t byte_idx = bitmap_idx / 8;
  uint8_t mask = 1 << (bitmap_idx % 8);
  
  bool free_alr = !(bitmap[byte_idx] & mask);

  if (!free_alr){
    bitmap[byte_idx] &= ~mask;
    // for alloc its bitmap[byte] |= mask
  
    root_fs->bgdt[group_n].unalloc_block_cnt++;
    root_fs->superblk->unalloc_blocks++;

    write_block(bgdt_blk_n, (uint16_t *) root_fs->bgdt);
    ata_write48_part(ATA_MASTER, root_fs->start_lba, root_fs->sector_count, superblk_pos / BYTES_PER_SECTOR, sizeof(struct ext2_superblock) / BYTES_PER_SECTOR, root_fs->sprblk_buf);
    write_block(root_fs->bgdt[group_n].block_bitmap_addr, (uint16_t *) bitmap);
  }

  kfree(bitmap_buf);
  return true;
}

bool inode_exists(uint32_t inode_n)
{
  uint32_t group_n = (inode_n - first_inode_n) / root_fs->inodes_per_grp;
  
  if (group_n >= root_fs->ngroups || inode_n > root_fs->superblk->inode_cnt) {
    kprintf(KPRINTF_RED "Invalid inode number");
    return false;
  }

  uint32_t bitmap_idx = (inode_n - first_inode_n) % root_fs->inodes_per_grp;

  uint16_t *bitmap_buf = (uint16_t *) kmalloc(BLOCK_SIZE);
  read_block(root_fs->bgdt[group_n].inode_bitmap_addr, bitmap_buf);

  uint8_t *bitmap = (uint8_t *)bitmap_buf;

  uint32_t byte_idx = bitmap_idx / 8;
  uint8_t mask = 1 << (bitmap_idx % 8);

  bool value = bitmap[byte_idx] & mask;
  
  kfree(bitmap_buf);

  return value;
}
