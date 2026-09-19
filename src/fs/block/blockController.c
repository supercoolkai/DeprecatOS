#include "fs/block/blockController.h"
#include "fs/ext2/inode.h"
#include "fs/ext2/superblock.h"
#include "fs/ext2/directoryEntry.h"
#include "fs/ext2/blockGroupDescriptor.h"
#include "drivers/disk/ata.h"
#include "memory/heap/kernelHeap.h"
#include "exceptions/exceptions.h"
#include "util/hex/hexPrinter.h"
#include "drivers/fb/fbController.h"
#include "fs/ext2/bitmapController.h"
#include "errors.h"
#include "drivers/timer/timerController.h"
#include "fs/ext2/directoryController.h"
#include <stdint.h>
#include <stdbool.h>

static struct ext2_superblock *superblk;
static uint16_t bgdt_buf[WORDS_PER_BLK];
static uint16_t sprblk_buf[sizeof(struct ext2_superblock) / 2];
static struct ext2_block_group_descriptor *bgdt;
static uint16_t sectors_per_blk = BLOCK_SIZE / BYTES_PER_SECTOR;
static uint32_t superblk_pos = EXT2_SUPERBLOCK_OFFSET;
static uint32_t bgdt_blk_n = 1;

static uint32_t inode_size;
static uint32_t inodes_per_grp;
static uint32_t blks_per_grp;
static uint32_t frags_per_grp;
static uint32_t ngroups;


// dum block reading function. very simple math u just 
// get lba get sectors then u get block.
void read_block(uint32_t block_n, uint16_t *buf)
{
  uint64_t lba = (uint64_t) (block_n * sectors_per_blk);

  ata_read48(ATA_MASTER, lba, sectors_per_blk, buf);
}

void write_block(uint32_t block_n, uint16_t *buf)
{
  uint64_t lba = (uint64_t) (block_n * sectors_per_blk);

  ata_write48(ATA_MASTER, lba, sectors_per_blk, buf);
}

static void indir_read_block(uint16_t **out_cursor, uint32_t *blocks_left, uint32_t block_n, int layer, uint16_t (*indirect_buf) [WORDS_PER_BLK])
{
  if (block_n == 0) 
    panic("KERNEL PANIC: BLOCK OUT OF RANGE");

  // baseo caseo
  if (layer == 0) {
    read_block(block_n, *out_cursor);
    *out_cursor += WORDS_PER_BLK;
    (*blocks_left)--;
    return;
  }

  // recurse
  read_block(block_n, indirect_buf[layer-1]);
  uint32_t *entries = (uint32_t *) indirect_buf[layer-1];
  for (int i = 0; i < BIT_32_PER_BLK && *blocks_left > 0; i++) {
    indir_read_block(out_cursor, blocks_left, entries[i], layer-1, indirect_buf);
  }
}

// init globals n stuff for future reference
void block_init(void)
{
  //read_block(1, sprblk_buf)
  ata_read48(ATA_MASTER, superblk_pos / BYTES_PER_SECTOR, sizeof(struct ext2_superblock) / BYTES_PER_SECTOR, sprblk_buf);
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
  uint16_t *buf = kmalloc(BLOCK_SIZE);
  uint32_t g = (inode_n - 1) / inodes_per_grp;
  uint32_t ind = (inode_n - 1) % inodes_per_grp;
  
  if (inode_n == 0 || inode_n > superblk->inode_cnt)
  {
    kfree(buf);
    return false;
  }

  uint32_t blk = bgdt[g].inode_start_addr + (ind * inode_size) / BLOCK_SIZE;
  uint32_t off = (ind * inode_size) % BLOCK_SIZE;
  
  read_block(blk, buf);
  *out = *(struct ext2_inode *)(((uint8_t *)buf) + off);
  kfree(buf);
  return true;
}

// reads the inputted inode
// self explanatory imo 
void read_inode(struct ext2_inode *inode, uint16_t *out)
{
  uint16_t (*indirect_buf)[WORDS_PER_BLK] = kmalloc(INDIRECT_PTR_LAYERS * BLOCK_SIZE);
  uint32_t blocks_left = (inode->size_lo + BLOCK_SIZE - 1) / BLOCK_SIZE;

  for (int i = 0; i < INODE_BLK_PTR_AMT && blocks_left > 0; i++) {
    indir_read_block(&out, &blocks_left, inode->dir_block_ptr[i], 0, indirect_buf);
  }
  
  if (blocks_left > 0) indir_read_block(&out, &blocks_left, inode->singly_indir_block_ptr, 1, indirect_buf);
  if (blocks_left > 0) indir_read_block(&out, &blocks_left,inode->doubly_indir_block_ptr, 2, indirect_buf);
  if (blocks_left > 0) indir_read_block(&out, &blocks_left, inode->triply_indir_block_ptr, 3, indirect_buf); 
  kfree(indirect_buf);
}

static void zero_block_buf(uint32_t *buf)
{
  for (uint32_t i = 0; i < BIT_32_PER_BLK; i++) {
    buf[i] = 0;
  }
}

// writes the inputted buf 
// into an inode and returns
// the inode number and inode struct
uint32_t write_inode(uint16_t *buf, uint32_t f_size, bool is_dir, struct ext2_inode *out)
{
  uint32_t inode_n = alloc_inode();
  
  if (inode_n == INODE_ERROR){
    return INODE_ERROR;
  }

  struct ext2_inode inode = {0};

  if (is_dir) {
    inode.type_and_perms_lo = INODE_DIR_TYPE;
  }
  else{
    inode.type_and_perms_lo = INODE_FILE_TYPE;
  }

  inode.size_lo = f_size;

  inode.creation_time = timer_get_tick();
  inode.last_access_time = inode.creation_time;
  inode.last_mod_time = inode.creation_time;

  inode.disk_sectors = (inode.size_lo / BYTES_PER_SECTOR);

  if (inode.size_lo % BYTES_PER_SECTOR != 0) {
    inode.disk_sectors++;
  }

  // no flags currently, only for extended features
  
  uint32_t blocks_left = (inode.size_lo + BLOCK_SIZE - 1) / BLOCK_SIZE;
  uint32_t block_n;
  uint32_t buf_cursor = 0;

  for (int i = 0 ; i < INODE_BLK_PTR_AMT && blocks_left > 0; i++) {
    block_n = alloc_block();
    if (block_n == BLOCK_ERROR) {
      free_inode(inode_n);
      return BLOCK_ERROR;
    }

    uint16_t *curr_blk = kmalloc(BLOCK_SIZE);
    zero_block_buf((uint32_t *) curr_blk);

    for (uint32_t j = 0; j < WORDS_PER_BLK; j++) {
      curr_blk[j] = buf[buf_cursor];
      buf_cursor++;
    }

    write_block(block_n, curr_blk);

    kfree(curr_blk);
    
    blocks_left--;

    inode.dir_block_ptr[i] = block_n;
  }

  uint32_t triple_ptr_temp = alloc_block();
  if (triple_ptr_temp == BLOCK_ERROR) { 
    free_inode(inode_n);
    return BLOCK_ERROR;
  }


  uint32_t *curr_triply_indir_blk = kmalloc(BLOCK_SIZE);
  zero_block_buf(curr_triply_indir_blk);
  bool flushed = false;
  bool has_double = false;
  bool has_triple = false;
  
  for (uint32_t triply_indir_blk_ptr_idx = 0; triply_indir_blk_ptr_idx < BLOCK_SIZE / 4 && blocks_left > 0; triply_indir_blk_ptr_idx ++ ){
    has_double = false;
    uint32_t doubly_indir_blk_ptr = alloc_block();
    if (doubly_indir_blk_ptr == BLOCK_ERROR) {
      free_inode(inode_n);
      return BLOCK_ERROR;
    }

    uint32_t *curr_doubly_indir_blk = kmalloc(BLOCK_SIZE);
    zero_block_buf(curr_doubly_indir_blk);
    
    for (uint32_t doubly_indir_blk_ptr_idx = 0; doubly_indir_blk_ptr_idx < BLOCK_SIZE / 4 && blocks_left > 0; doubly_indir_blk_ptr_idx ++){
      uint32_t indir_blk_ptr = alloc_block();
      
      if (indir_blk_ptr == BLOCK_ERROR) {
        free_inode(inode_n);
        return BLOCK_ERROR;
      }

      uint32_t *curr_indir_blk = kmalloc(BLOCK_SIZE);
      zero_block_buf(curr_indir_blk);
      for (uint32_t indir_blk_ptr_idx = 0; indir_blk_ptr_idx < BLOCK_SIZE / 4 && blocks_left > 0; indir_blk_ptr_idx ++){
        block_n = alloc_block();
        if (block_n == BLOCK_ERROR) {
          // do note that this leaves
          // a ton of allocated blocks
          // on error. best u can do
          // is leave the blocks but 
          // unallocate the inode
      
          free_inode(inode_n);
          return BLOCK_ERROR;
        }

        uint16_t *curr_blk = kmalloc(BLOCK_SIZE);
        zero_block_buf((uint32_t *) curr_blk);

        for (int j = 0; j < WORDS_PER_BLK; j++) {
          curr_blk[j] = buf[buf_cursor];
          buf_cursor++;
        }

        write_block(block_n, curr_blk);

        kfree(curr_blk);

        curr_indir_blk[indir_blk_ptr_idx] = block_n;
    
        blocks_left--;
      }

      if(blocks_left > 0 || !flushed){
        if (!flushed && blocks_left <= 0) {
          write_block(indir_blk_ptr, (uint16_t *) curr_indir_blk);
          flushed = true;
        }

        else if (!flushed) {
          write_block(indir_blk_ptr, (uint16_t *) curr_indir_blk);
        }

        if (doubly_indir_blk_ptr_idx == 0) {
          inode.singly_indir_block_ptr = indir_blk_ptr;
          inode.disk_sectors+=sectors_per_blk;
          kfree(curr_indir_blk);
          continue;
        }
        kfree(curr_indir_blk);
        curr_doubly_indir_blk[doubly_indir_blk_ptr_idx-1] = indir_blk_ptr;
        inode.disk_sectors+=sectors_per_blk;
        has_double = true;
      }
    }
    

    if (!has_double) {
      free_block(doubly_indir_blk_ptr);
    }
    
    if (has_double){
      write_block(doubly_indir_blk_ptr, (uint16_t *) curr_doubly_indir_blk);
      
      if (triply_indir_blk_ptr_idx != 0){
        curr_triply_indir_blk[triply_indir_blk_ptr_idx - 1] = doubly_indir_blk_ptr;
        has_triple = true;
        inode.disk_sectors+=sectors_per_blk;
      }
    }

    if (triply_indir_blk_ptr_idx == 0  && has_double) {
      inode.doubly_indir_block_ptr = doubly_indir_blk_ptr;
      inode.disk_sectors+=sectors_per_blk;
      kfree(curr_doubly_indir_blk);
      continue;
    }
      
    kfree(curr_doubly_indir_blk);
    
  }


  if (has_triple) {
    inode.triply_indir_block_ptr = triple_ptr_temp;
    write_block(inode.triply_indir_block_ptr, (uint16_t *) curr_triply_indir_blk);
    inode.disk_sectors+=sectors_per_blk;
  }
  
  kfree(curr_triply_indir_blk);

  if (!has_triple) 
    free_block(triple_ptr_temp);
    
  *out = inode;

  return inode_n;
}
