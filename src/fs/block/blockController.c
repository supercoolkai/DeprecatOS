#include "fs/block/blockController.h"
#include "fs/ext2/inode.h"
#include "fs/ext2/superblock.h"
#include "fs/ext2/directoryEntry.h"
#include "fs/ext2/blockGroupDescriptor.h"
#include "drivers/disk/ata.h"
#include "exceptions/exceptions.h"
#include "util/hex/hexPrinter.h"
#include "drivers/fb/fbController.h"
#include <stdint.h>
#include <stdbool.h>

static struct ext2_superblock *superblk;
static uint16_t bgdt_buf[WORDS_PER_BLK];
static uint16_t sprblk_buf[sizeof(struct ext2_superblock) / 2];
static struct ext2_block_group_descriptor *bgdt;
static uint16_t sectors_per_blk = BLOCK_SIZE / READ_48_AMT;
static uint32_t superblk_pos = 1024;
static uint32_t bgdt_blk_n = 1;

static uint16_t buf[WORDS_PER_BLK];

static uint32_t inode_size;
static uint32_t inodes_per_grp;
static uint32_t blks_per_grp;
static uint32_t frags_per_grp;
static uint32_t ngroups;

static uint16_t indirect_buf[3][WORDS_PER_BLK];

static uint16_t *out_cursor;
static uint32_t blocks_left;

// dum block reading function. very simple math u just 
// get lba get sectors then u get block.
// 
// for the functions which need to get access to a block. currentyl only here 
// so static
void read_block(uint32_t block_n, uint16_t *buf)
{
  uint64_t lba = (uint64_t) (block_n * sectors_per_blk);

  ata_read48(ATA_MASTER, lba, sectors_per_blk, buf);
}

static void indir_read_block(uint32_t block_n, int layer)
{
  if (block_n == 0) 
    panic("KERNEL PANIC: BLOCK OUT OF RANGE");

  // baseo caseo
  if (layer == 0) {
    read_block(block_n, out_cursor);
    out_cursor += WORDS_PER_BLK;
    blocks_left--;
    return;
  }

  // recurse
  read_block(block_n, indirect_buf[layer-1]);
  uint32_t *entries = (uint32_t *) indirect_buf[layer-1];
  for (int i = 0; i < BIT_32_PER_BLK && blocks_left > 0; i++) {
    indir_read_block(entries[i], layer-1);
  }
}

// init globals n stuff for future reference
void block_init(void)
{
  //read_block(1, sprblk_buf)
  ata_read48(ATA_MASTER, superblk_pos / READ_48_AMT, sizeof(struct ext2_superblock) / 512, sprblk_buf);
  read_block(bgdt_blk_n, bgdt_buf);

  superblk = (struct ext2_superblock *) sprblk_buf;
  bgdt = (struct ext2_block_group_descriptor *) bgdt_buf;

  inodes_per_grp = superblk->inodes_per_group;
  blks_per_grp = superblk->blocks_per_group;
  frags_per_grp = superblk->frags_per_group;
  inode_size = superblk->inode_size;

  ngroups = (superblk->block_cnt + blks_per_grp - 1) / blks_per_grp;
}

// get inode  part one of pipeline need 
// read_inode to read contents
bool get_inode(uint32_t inode_n, struct ext2_inode *out)
{
  uint32_t g = (inode_n - 1) / inodes_per_grp;
  uint32_t ind = (inode_n - 1) % inodes_per_grp;
  
  if (inode_n == 0 || inode_n > superblk->inode_cnt)
  {
    return false;
  }

  uint32_t blk = bgdt[g].inode_start_addr + (ind * inode_size) / BLOCK_SIZE;
  uint32_t off = (ind * inode_size) % BLOCK_SIZE;
  
  read_block(blk, buf);
  *out = *(struct ext2_inode *)(((uint8_t *)buf) + off);
  return true;
}

// reads the inputted inode
// self explanatory imo 
void read_inode(struct ext2_inode *inode, uint16_t *out)
{
  out_cursor = out;
  blocks_left = (inode->size_lo + BLOCK_SIZE - 1) / BLOCK_SIZE;

  for (int i = 0; i < INODE_BLK_PTR_AMT && blocks_left > 0; i++) {
    indir_read_block(inode->dir_block_ptr[i], 0);
  }
  
  if (blocks_left > 0) indir_read_block(inode->singly_indir_block_ptr, 1);
  if (blocks_left > 0) indir_read_block(inode->doubly_indir_block_ptr, 2);
  if (blocks_left > 0) indir_read_block(inode->triply_indir_block_ptr, 3); 
}
