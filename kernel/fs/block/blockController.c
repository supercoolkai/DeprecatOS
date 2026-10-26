#include "fs/block/blockController.h"
#include "fs/ext2/inode.h"
#include "fs/ext2/superblock.h"
#include "fs/ext2/directoryEntry.h"
#include "fs/ext2/blockGroupDescriptor.h"
#include "drivers/disk/ata.h"
#include "memory/heap/kernelHeap.h"
#include "exceptions/exceptions.h"
#include "hex/hexPrinter.h"
#include "drivers/fb/fbController.h"
#include "fs/ext2/bitmapController.h"
#include "errors.h"
#include "drivers/timer/timerController.h"
#include "fs/ext2/directoryController.h"
#include "min/min.h"
#include "max/max.h"
#include "drivers/disk/partition.h"
#include "kprintf/kprintf.h"
#include "streq/streq.h"
#include <stdint.h>
#include <stdbool.h>

#define INCOMPAT_FEATURES_SUPPORT_BYTE 0x2
#define RO_FEATURES_SUPPORT_BYTE 0x3

static uint16_t sectors_per_blk = BLOCK_SIZE / BYTES_PER_SECTOR;
static uint32_t superblk_pos = EXT2_SUPERBLOCK_OFFSET;
static uint32_t bgdt_blk_n = 1;
static uint32_t first_inode_n = 1;

struct fs_info fs_entries[MAX_MOUNTS];
struct fs_info *root_fs;

// dum block reading function. very simple math u just 
// get lba get sectors then u get block.
void read_block(struct fs_info *fs, uint32_t block_n, uint16_t *buf)
{
  uint64_t lba = (uint64_t) (block_n * sectors_per_blk);

  ata_read48_part(fs->drive->identify_magic, fs->start_lba, fs->sector_count, lba, sectors_per_blk, buf);
}

void write_block(struct fs_info *fs, uint32_t block_n, uint16_t *buf)
{
  uint64_t lba = (uint64_t) (block_n * sectors_per_blk);

  ata_write48_part(fs->drive->identify_magic, fs->start_lba, fs->sector_count, lba, sectors_per_blk, buf);
}

static void indir_read_block(struct fs_info *fs, uint16_t **out_cursor, uint32_t *blocks_left, uint32_t block_n, int layer, uint16_t (*indirect_buf) [WORDS_PER_BLK])
{
  if (block_n == 0){
    uint32_t span = 1;
    for (int l = 0; l < layer; l++){
      span *= BIT_32_PER_BLK;
    }

    span = min(span, *blocks_left);

    for (uint32_t w = 0; w < span * WORDS_PER_BLK; w++) {
      (*out_cursor)[w] = 0;
    }

    *out_cursor += span * WORDS_PER_BLK;
    *blocks_left -= span;
    return;
  }

  // baseo caseo
  if (layer == 0) {
    read_block(fs, block_n, *out_cursor);
    *out_cursor += WORDS_PER_BLK;
    (*blocks_left)--;
    return;
  }

  // recurse
  read_block(fs, block_n, indirect_buf[layer-1]);
  uint32_t *entries = (uint32_t *) indirect_buf[layer-1];
  for (int i = 0; i < BIT_32_PER_BLK && *blocks_left > 0; i++) {
    indir_read_block(fs, out_cursor, blocks_left, entries[i], layer-1, indirect_buf);
  }
}

static void indir_free_block(struct fs_info *fs, uint32_t block_n, uint32_t layer, uint32_t (*indirect_buf)[BIT_32_PER_BLK])
{
  if (block_n == 0)
    return;

  if (layer == 0) {
    free_block(fs, block_n);
    return;
  }

  read_block(fs, block_n, (uint16_t *) indirect_buf[layer-1]);
  uint32_t *entries = (uint32_t *) indirect_buf[layer-1];
  for(int i = 0; i < BIT_32_PER_BLK; i++) {
    indir_free_block(fs, entries[i], layer-1, indirect_buf);
  }
  free_block(fs, block_n);
}

static bool fill_partition_entry(uint8_t drive_magic, uint32_t part)
{
  struct ata_drive drive;
  struct ata_drive *drive_ptr;
  if (drive_magic == ATA_MASTER){
    drive = master_drive;
    drive_ptr = &master_drive;
  }
  else if (drive_magic == ATA_SLAVE){
    drive = slave_drive;
    drive_ptr = &slave_drive;
  }
  else
    return false;

  struct drive_mbr *mbr;
  uint8_t *part_entry;
  uint32_t mbr_part_n = part % PARTS_PER_DRIVE;

  mbr = read_mbr(drive);

  if (mbr == NULL){
    if (drive_magic == ATA_MASTER)
      panic("KERNEL PANIC: master drive has no MBR");
    return false;
  }

  if (!(mbr->partition_magic_1 == 0x55 && mbr->partition_magic_2 == 0xAA)){
    if (drive_magic == ATA_MASTER)
      panic("KERNEL PANIC: master drive has a bad MBR signature");
    kprintf(KPRINTF_YELLOW "WARNING: slave drive has a bad MBR signature\n" KPRINTF_RESET);
    kfree(mbr);
    return false;
  }

  part_entry = (uint8_t *)(((uint8_t *) mbr) + DRIVE_UNUSED_SIZE + PART_ENTRY_SIZE * mbr_part_n);
  uint8_t part_good = *((uint8_t*)(part_entry + PART_GOOD_OFFSET));
  
  if (part_good == 0x05 || part_good == 0x0F){
    if (drive_magic == ATA_MASTER)
      kprintf(KPRINTF_YELLOW "WARNING: master drive has an extended partition, skipped\n" KPRINTF_RESET);
    else
      kprintf(KPRINTF_YELLOW "WARNING: slave drive has an extended partition, skipped\n" KPRINTF_RESET);
    kfree(mbr);
    return true;
  }

  if (part_good != 0x83){
    kfree(mbr);
    return true;
  }

  fs_entries[part].drive = drive_ptr;
  fs_entries[part].start_lba = 
    (uint32_t)*(part_entry + PART_LBA_OFFSET) |
    ((uint32_t)*(part_entry + PART_LBA_OFFSET + 1)) << 8 |
    ((uint32_t)*(part_entry + PART_LBA_OFFSET + 2)) << 16 |
    ((uint32_t)*(part_entry + PART_LBA_OFFSET + 3)) << 24;
  
  fs_entries[part].sector_count = 
    (uint32_t)*(part_entry + PART_SECTOR_COUNT_OFFSET) |
    ((uint32_t)*(part_entry + PART_SECTOR_COUNT_OFFSET + 1)) << 8 |
    ((uint32_t)*(part_entry + PART_SECTOR_COUNT_OFFSET + 2)) << 16 |
    ((uint32_t)*(part_entry + PART_SECTOR_COUNT_OFFSET + 3)) << 24;
  
  if (fs_entries[part].sector_count == 0 || fs_entries[part].start_lba + fs_entries[part].sector_count > drive.lba_sector_count){
    if (drive_magic == ATA_MASTER)
      kprintf(KPRINTF_YELLOW "WARNING: master drive partition is out of bounds, skipped\n" KPRINTF_RESET);
    else
      kprintf(KPRINTF_YELLOW "WARNING: slave drive partition is out of bounds, skipped\n" KPRINTF_RESET);
    fs_entries[part].sector_count = 0;
    kfree(mbr);
    return true;
  }

  ata_read48_part(fs_entries[part].drive->identify_magic,
                  fs_entries[part].start_lba,
                  fs_entries[part].sector_count,
                  superblk_pos / BYTES_PER_SECTOR,
                  sizeof(struct ext2_superblock) / BYTES_PER_SECTOR,
                  fs_entries[part].sprblk_buf);

  fs_entries[part].superblk = (struct ext2_superblock *) fs_entries[part].sprblk_buf;

  if (!(fs_entries[part].superblk->ext2_signature == 0xEF53 && (1024 << fs_entries[part].superblk->block_size_adjusted) == 4096)){
    if (drive_magic == ATA_MASTER)
      kprintf(KPRINTF_YELLOW "WARNING: master drive partition is not ext2 with 4K blocks, skipped\n" KPRINTF_RESET);
    else
      kprintf(KPRINTF_YELLOW "WARNING: slave drive partition is not ext2 with 4K blocks, skipped\n" KPRINTF_RESET);
    fs_entries[part].sector_count = 0;
    fs_entries[part].superblk = NULL;
    kfree(mbr);
    return true;
  }

  if (fs_entries[part].superblk->req_features_present & ~INCOMPAT_FEATURES_SUPPORT_BYTE) {
    kprintf(KPRINTF_YELLOW "WARNING: partition uses incompatible features, skipped\n" KPRINTF_RESET);
    fs_entries[part].sector_count = 0;
    fs_entries[part].superblk = NULL;
    kfree(mbr);
    return true;
  }

  if (fs_entries[part].superblk->features_unsupport_ro & ~RO_FEATURES_SUPPORT_BYTE) {
    kprintf(KPRINTF_YELLOW "WARNING: partition uses features this kernel cannot write, skipped\n" KPRINTF_RESET);
    fs_entries[part].sector_count = 0;
    fs_entries[part].superblk = NULL;
    kfree(mbr);
    return true;
  }

  ata_read48_part(fs_entries[part].drive->identify_magic,
                  fs_entries[part].start_lba,
                  fs_entries[part].sector_count,
                  bgdt_blk_n * sectors_per_blk,
                  sectors_per_blk,
                  fs_entries[part].bgdt_buf);

  fs_entries[part].bgdt = (struct ext2_block_group_descriptor *) fs_entries[part].bgdt_buf;

  fs_entries[part].inodes_per_grp = fs_entries[part].superblk->inodes_per_group;
  fs_entries[part].blks_per_grp = fs_entries[part].superblk->blocks_per_group;
  fs_entries[part].frags_per_grp = fs_entries[part].superblk->frags_per_group;
  fs_entries[part].inode_size = fs_entries[part].superblk->inode_size;
  
  fs_entries[part].ngroups = 
    (fs_entries[part].superblk->block_cnt +
    fs_entries[part].blks_per_grp - 1) / 
    fs_entries[part].blks_per_grp;

  for (uint32_t j = 0; j < 16; j++) {
    fs_entries[part].label[j] = (char) fs_entries[part].superblk->vol_name[j];
  }
  fs_entries[part].label[FS_INFO_LABEL_LEN - 1] = '\0';

  kfree(mbr);
  return true;
}

// init globals n stuff for future reference
void block_init(void)
{
  for (uint32_t part = 0; part < MAX_MOUNTS; part++) {
    uint8_t drive_magic = part / PARTS_PER_DRIVE == 0 ? ATA_MASTER : ATA_SLAVE;

    if (!fill_partition_entry(drive_magic, part))
      break;
  }

  uint32_t root_part;
  for (root_part = 0; root_part < MAX_MOUNTS; root_part++) {
    if (fs_entries[root_part].sector_count != 0 && 
      streq(fs_entries[root_part].label, "deprecatOS"))
      break;
  }

  if (root_part == MAX_MOUNTS) {
    panic("KERNEL PANIC: bad drives");
  }

  root_fs = &fs_entries[root_part];
}

// get inode  part one of pipeline need 
// read_inode to read contents
bool get_inode(struct fs_info *fs, uint32_t inode_n, struct ext2_inode *out)
{
  uint16_t *buf = kmalloc(BLOCK_SIZE);
  uint32_t g = (inode_n - 1) / fs->inodes_per_grp;
  uint32_t ind = (inode_n - 1) % fs->inodes_per_grp;
  
  if (inode_n == 0 || inode_n > fs->superblk->inode_cnt)
  {
    kfree(buf);
    return false;
  }

  uint32_t blk = fs->bgdt[g].inode_table_start_addr + (ind * fs->inode_size) / BLOCK_SIZE;
  uint32_t off = (ind * fs->inode_size) % BLOCK_SIZE;
  
  read_block(fs, blk, buf);
  *out = *(struct ext2_inode *)(((uint8_t *)buf) + off);
  kfree(buf);
  return true;
}

// set inode
bool set_inode(struct fs_info *fs, uint32_t inode_n, struct ext2_inode *in)
{
  uint16_t *buf = kmalloc(BLOCK_SIZE);
  uint32_t g = (inode_n - 1) / fs->inodes_per_grp;
  uint32_t ind = (inode_n - 1) % fs->inodes_per_grp;
  
  if (inode_n == 0 || inode_n > fs->superblk->inode_cnt)
  {
    kfree(buf);
    return false;
  }

  uint32_t blk = fs->bgdt[g].inode_table_start_addr + (ind * fs->inode_size) / BLOCK_SIZE;
  uint32_t off = (ind * fs->inode_size) % BLOCK_SIZE;
  
  read_block(fs, blk, buf);
  *(struct ext2_inode *)(((uint8_t *)buf) + off) = *in;
  write_block(fs, blk, buf);
  kfree(buf);
  return true;
}

static void zero_block_buf(uint32_t *buf)
{
  for (uint32_t i = 0; i < BIT_32_PER_BLK; i++) {
    buf[i] = 0;
  }
}

bool delete_inode(struct fs_info *fs, uint32_t inode_n)
{
  struct ext2_inode inode;
  if (!get_inode(fs, inode_n, &inode))
    return false;

  uint32_t (*unwind_buf)[BIT_32_PER_BLK] = kmalloc(INDIRECT_PTR_LAYERS * BLOCK_SIZE);
  if (inode.disk_sectors > 0)
  {
    for (int i = 0; i < INODE_BLK_PTR_AMT; i++) {
      indir_free_block(fs, inode.dir_block_ptr[i], 0, unwind_buf);
    }

    indir_free_block(fs, inode.singly_indir_block_ptr, 1, unwind_buf);
    indir_free_block(fs, inode.doubly_indir_block_ptr, 2, unwind_buf);
    indir_free_block(fs, inode.triply_indir_block_ptr, 3, unwind_buf);
  }

  inode.hard_link_cnt = 0;
  inode.deletion_time = timer_get_epoch_sec();

  if(!set_inode(fs, inode_n, &inode)){
    kfree(unwind_buf);
    return false;
  }
  
  if((inode.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_DIR_TYPE){
    uint32_t group_n = (inode_n - first_inode_n) / fs->inodes_per_grp;
    fs->bgdt[group_n].dir_cnt--;
    write_block(fs, bgdt_blk_n, (uint16_t *) fs->bgdt);
  }

  if (!free_inode(fs, inode_n)){
    kfree(unwind_buf);
    return false;
  }
  
  kfree(unwind_buf);
  return true;
}

uint32_t mint_inode(struct fs_info *fs, uint32_t f_size, uint32_t type, struct ext2_inode *out)
{
  uint32_t inode_n = alloc_inode(fs);

  if (inode_n == INODE_ERROR)
    return INODE_ERROR;

  struct ext2_inode inode = {0};

  inode.type_and_perms_lo = type;

  inode.size_lo = f_size;

  inode.creation_time = timer_get_epoch_sec();
  inode.last_access_time = inode.creation_time;
  inode.last_mod_time = inode.creation_time;

  //inode.disk_sectors = ((inode.size_lo + BLOCK_SIZE - 1) / BLOCK_SIZE) * sectors_per_blk;
  inode.disk_sectors = 0;

  *out = inode;
  return inode_n;
}

// writes the inputted buf 
// into an inode and returns
// the inode struct
uint32_t write_inode(struct fs_info *fs, uint16_t *buf, struct ext2_inode *inode)
{
  // no flags currently, only for extended features
  for (int i = 0; i < INODE_BLK_PTR_AMT; i++) {
    inode->dir_block_ptr[i] = 0;
  }

  inode->singly_indir_block_ptr = 0;
  inode->doubly_indir_block_ptr = 0;
  inode->triply_indir_block_ptr = 0;
  
  uint32_t blocks_left = (inode->size_lo + BLOCK_SIZE - 1) / BLOCK_SIZE;
  uint32_t block_n;
  uint32_t buf_cursor = 0;

  for (int i = 0 ; i < INODE_BLK_PTR_AMT && blocks_left > 0; i++) {
    block_n = alloc_block(fs);
    if (block_n == BLOCK_ERROR) {
      uint32_t (*unwind_buf)[BIT_32_PER_BLK] = kmalloc(INDIRECT_PTR_LAYERS * BLOCK_SIZE);
      for (int j = 0; j < INODE_BLK_PTR_AMT; j++) {
        indir_free_block(fs, inode->dir_block_ptr[j], 0, unwind_buf);
      }

      kfree(unwind_buf);
      return BLOCK_ERROR;
    }
    inode->disk_sectors+=sectors_per_blk;

    uint16_t *curr_blk = kmalloc(BLOCK_SIZE);
    zero_block_buf((uint32_t *) curr_blk);

    
    uint32_t copy_bytes = min(inode->size_lo - buf_cursor * 2, BLOCK_SIZE);
    for (uint32_t j = 0; j < copy_bytes / 2; j++) {
      curr_blk[j] = buf[buf_cursor];
      buf_cursor++;
    }

    if (copy_bytes % 2) {
      ((uint8_t *) curr_blk)[copy_bytes - 1] = ((uint8_t *)buf)[buf_cursor * 2];
    }

    write_block(fs, block_n, curr_blk);

    kfree(curr_blk);
    
    blocks_left--;

    inode->dir_block_ptr[i] = block_n;
  }

  uint32_t triple_ptr_temp = alloc_block(fs);
  if (triple_ptr_temp == BLOCK_ERROR) { 

    uint32_t (*unwind_buf)[BIT_32_PER_BLK] = kmalloc(INDIRECT_PTR_LAYERS * BLOCK_SIZE);
    for (int i = 0; i < INODE_BLK_PTR_AMT; i++) {
      indir_free_block(fs, inode->dir_block_ptr[i], 0, unwind_buf);
    }
    
    kfree(unwind_buf);
    return BLOCK_ERROR;
  }


  uint32_t *curr_triply_indir_blk = kmalloc(BLOCK_SIZE);
  zero_block_buf(curr_triply_indir_blk);
  bool flushed = false;
  bool has_double = false;
  bool has_triple = false;
  
  for (uint32_t triply_indir_blk_ptr_idx = 0; triply_indir_blk_ptr_idx < BLOCK_SIZE / 4 && blocks_left > 0; triply_indir_blk_ptr_idx ++ ){
    has_double = false;
    uint32_t doubly_indir_blk_ptr = alloc_block(fs);
    if (doubly_indir_blk_ptr == BLOCK_ERROR) {
      uint32_t (*unwind_buf)[BIT_32_PER_BLK] = kmalloc(INDIRECT_PTR_LAYERS * BLOCK_SIZE);
      for (int i = 0; i < INODE_BLK_PTR_AMT; i++) {
        indir_free_block(fs, inode->dir_block_ptr[i], 0, unwind_buf);
      }

      for (int i = 0; i < BIT_32_PER_BLK; i++) {
        indir_free_block(fs, curr_triply_indir_blk[i], 2, unwind_buf);
      }
      
      indir_free_block(fs, inode->singly_indir_block_ptr, 1, unwind_buf);
      indir_free_block(fs, inode->doubly_indir_block_ptr, 2, unwind_buf);

      kfree(unwind_buf);
      kfree(curr_triply_indir_blk);
      free_block(fs, triple_ptr_temp);
      return BLOCK_ERROR;
    }

    uint32_t *curr_doubly_indir_blk = kmalloc(BLOCK_SIZE);
    zero_block_buf(curr_doubly_indir_blk);
    
    for (uint32_t doubly_indir_blk_ptr_idx = 0; doubly_indir_blk_ptr_idx < BLOCK_SIZE / 4 && blocks_left > 0; doubly_indir_blk_ptr_idx ++){
      uint32_t indir_blk_ptr = alloc_block(fs);
      
      if (indir_blk_ptr == BLOCK_ERROR) {
        uint32_t (*unwind_buf)[BIT_32_PER_BLK] = kmalloc(INDIRECT_PTR_LAYERS * BLOCK_SIZE);
        for (int i = 0; i < INODE_BLK_PTR_AMT; i++) {
        indir_free_block(fs, inode->dir_block_ptr[i], 0, unwind_buf);
        }
        
        indir_free_block(fs, inode->singly_indir_block_ptr, 1, unwind_buf);
        indir_free_block(fs, inode->doubly_indir_block_ptr, 2, unwind_buf);

        for (int i = 0; i < BIT_32_PER_BLK; i++) {
          indir_free_block(fs, curr_triply_indir_blk[i], 2, unwind_buf);
        }

        for (int i = 0; i < BIT_32_PER_BLK; i++) {
          indir_free_block(fs, curr_doubly_indir_blk[i], 1, unwind_buf);
        }

        kfree(unwind_buf);
        kfree(curr_triply_indir_blk);
        kfree(curr_doubly_indir_blk);
        free_block(fs, doubly_indir_blk_ptr);
        free_block(fs, triple_ptr_temp);
        return BLOCK_ERROR;
      }
      inode->disk_sectors+=sectors_per_blk;

      uint32_t *curr_indir_blk = kmalloc(BLOCK_SIZE);
      zero_block_buf(curr_indir_blk);
      for (uint32_t indir_blk_ptr_idx = 0; indir_blk_ptr_idx < BLOCK_SIZE / 4 && blocks_left > 0; indir_blk_ptr_idx ++){
        block_n = alloc_block(fs);
        if (block_n == BLOCK_ERROR) {
          uint32_t (*unwind_buf)[BIT_32_PER_BLK] = kmalloc(INDIRECT_PTR_LAYERS * BLOCK_SIZE);
          
          for (int i = 0; i < INODE_BLK_PTR_AMT; i++) {
            indir_free_block(fs, inode->dir_block_ptr[i], 0, unwind_buf);
          }
          indir_free_block(fs, inode->singly_indir_block_ptr, 1, unwind_buf);
          indir_free_block(fs, inode->doubly_indir_block_ptr, 2, unwind_buf);
          
          for (int i = 0; i < BIT_32_PER_BLK; i++) {
            indir_free_block(fs, curr_triply_indir_blk[i], 2, unwind_buf);
          }

          for (int i = 0; i < BIT_32_PER_BLK; i++) {
            indir_free_block(fs, curr_doubly_indir_blk[i], 1, unwind_buf);
          }

          for (int i = 0; i < BIT_32_PER_BLK; i++) {
            indir_free_block(fs, curr_indir_blk[i], 0, unwind_buf);
          }


          kfree(unwind_buf);
          kfree(curr_triply_indir_blk);
          kfree(curr_doubly_indir_blk);
          kfree(curr_indir_blk);
          free_block(fs, indir_blk_ptr);
          free_block(fs, doubly_indir_blk_ptr);
          free_block(fs, triple_ptr_temp);
          return BLOCK_ERROR;
        }

        inode->disk_sectors+=sectors_per_blk;

        uint16_t *curr_blk = kmalloc(BLOCK_SIZE);
        zero_block_buf((uint32_t *) curr_blk);

        uint32_t copy_bytes = min(inode->size_lo - buf_cursor * 2, BLOCK_SIZE);
        for (int j = 0; j < copy_bytes / 2; j++) {
          curr_blk[j] = buf[buf_cursor];
          buf_cursor++;
        }

        if (copy_bytes % 2) {
          ((uint8_t *) curr_blk)[copy_bytes - 1] = ((uint8_t *)buf)[buf_cursor * 2];
        }

        write_block(fs, block_n, curr_blk);

        kfree(curr_blk);

        curr_indir_blk[indir_blk_ptr_idx] = block_n;
    
        blocks_left--;
      }

      if(blocks_left > 0 || !flushed){
        if (!flushed && blocks_left <= 0) {
          write_block(fs, indir_blk_ptr, (uint16_t *) curr_indir_blk);
          flushed = true;
        }

        else if (!flushed) {
          write_block(fs, indir_blk_ptr, (uint16_t *) curr_indir_blk);
        }

        if (doubly_indir_blk_ptr_idx == 0) {
          inode->singly_indir_block_ptr = indir_blk_ptr;
          kfree(curr_indir_blk);
          continue;
        }
        kfree(curr_indir_blk);
        curr_doubly_indir_blk[doubly_indir_blk_ptr_idx-1] = indir_blk_ptr;
        has_double = true;
      }
    }
    

    if (!has_double) {
      free_block(fs, doubly_indir_blk_ptr);
    }
    
    if (has_double){
      write_block(fs, doubly_indir_blk_ptr, (uint16_t *) curr_doubly_indir_blk);
      
      if (triply_indir_blk_ptr_idx != 0){
        curr_triply_indir_blk[triply_indir_blk_ptr_idx - 1] = doubly_indir_blk_ptr;
        has_triple = true;
        inode->disk_sectors+=sectors_per_blk;
      }
    }

    if (triply_indir_blk_ptr_idx == 0  && has_double) {
      inode->doubly_indir_block_ptr = doubly_indir_blk_ptr;
      inode->disk_sectors+=sectors_per_blk;
      kfree(curr_doubly_indir_blk);
      continue;
    }
      
    kfree(curr_doubly_indir_blk);
    
  }


  if (has_triple) {
    inode->triply_indir_block_ptr = triple_ptr_temp;
    write_block(fs, inode->triply_indir_block_ptr, (uint16_t *) curr_triply_indir_blk);
    inode->disk_sectors+=sectors_per_blk;
  }
  
  kfree(curr_triply_indir_blk);

  if (!has_triple){
    free_block(fs, triple_ptr_temp);
  }
    
  return INODE_WRITE_SUCCESS;
}

uint32_t replace_inode(struct fs_info *fs, uint32_t inode_n, uint16_t *buf, uint32_t f_size, struct ext2_inode *out)
{
  // delete_inode() but like
  // it doesnt reset the inode

  struct ext2_inode inode;
  if (!get_inode(fs, inode_n, &inode))
    return INODE_ERROR;

  uint32_t (*unwind_buf)[BIT_32_PER_BLK] = kmalloc(INDIRECT_PTR_LAYERS * BLOCK_SIZE);
  
  for (int i = 0; i < INODE_BLK_PTR_AMT; i++) {
    indir_free_block(fs, inode.dir_block_ptr[i], 0, unwind_buf);
  }

  indir_free_block(fs, inode.singly_indir_block_ptr, 1, unwind_buf);
  indir_free_block(fs, inode.doubly_indir_block_ptr, 2, unwind_buf);
  indir_free_block(fs, inode.triply_indir_block_ptr, 3, unwind_buf);

  kfree(unwind_buf);

  // now just write with the given information
  uint32_t old_last_mod_time = inode.last_mod_time;

  inode.size_lo = f_size;
  inode.disk_sectors = 0;
  inode.last_mod_time = timer_get_epoch_sec();

  if (write_inode(fs, buf, &inode) != INODE_WRITE_SUCCESS){

    for (int i = 0; i < INODE_BLK_PTR_AMT; i++) {
      inode.dir_block_ptr[i] = 0;
    }

    inode.singly_indir_block_ptr = 0;
    inode.doubly_indir_block_ptr = 0;
    inode.triply_indir_block_ptr = 0;

    inode.size_lo = 0;
    inode.disk_sectors = 0;
    inode.last_mod_time = old_last_mod_time;

    if (!set_inode(fs, inode_n, &inode)) {
      panic("KERNEL PANIC: Error when trying to recover from a bad write_inode(), disk corruption occured!");
    }
    
    return INODE_ERROR;
  }

  if (!set_inode(fs, inode_n, &inode)){
    return INODE_ERROR;
  }
  
  *out = inode;

  return INODE_WRITE_SUCCESS;
}

// modes: 
// LOOKUP_MODE: looks up, self explanatory. Read only
// ALLOC_MODE: looks up, but if not existent then allocates a block for it and points it to that
// FREE_MODE: like LOOKUP_MODE, but disk_blk_out = the old value of inode->dir_block_ptr. it zeroes it tho

// Do note that alloc_cnt becomes free_cnt when FREE_MODE is enabled

// YES this functoin can be written like 50 lines
// NO im not going to rewrite it like that
uint32_t map_file_block(struct fs_info *fs, struct ext2_inode *inode, uint32_t file_blk_idx, uint32_t mode, uint32_t *disk_blk_out, uint32_t *alloc_cnt)
{
  if (file_blk_idx < INODE_BLK_PTR_AMT) {
    if (inode->dir_block_ptr[file_blk_idx] == 0) {
      if (mode != ALLOC_MODE){
        if (mode == LOOKUP_MODE || mode == FREE_MODE)
          return EMPTY_BLOCK;
        return INODE_ERROR;
      }
      inode->dir_block_ptr[file_blk_idx] = alloc_block(fs);
      if (!inode->dir_block_ptr[file_blk_idx])
        return BLOCK_ERROR;
      if (alloc_cnt) (*alloc_cnt)++;
    }

    *disk_blk_out = inode->dir_block_ptr[file_blk_idx];
    if (mode == FREE_MODE){
      if (alloc_cnt) (*alloc_cnt)++;
      inode->dir_block_ptr[file_blk_idx] = 0;
    }
  }
  else {
    uint32_t indir_relative_idx = file_blk_idx - INODE_BLK_PTR_AMT;
    
    if (indir_relative_idx < BIT_32_PER_BLK) {
      bool singly_ptr_minted = false;
      if (inode->singly_indir_block_ptr == 0) {
        if (mode != ALLOC_MODE){
          if (mode == LOOKUP_MODE || mode == FREE_MODE)
            return EMPTY_BLOCK;
          return INODE_ERROR;
        }
        inode->singly_indir_block_ptr = alloc_block(fs);
        if (!inode->singly_indir_block_ptr)
          return BLOCK_ERROR;

        uint16_t *temp_blk_buf = kmalloc(BLOCK_SIZE);
        zero_block_buf((uint32_t *) temp_blk_buf);
        write_block(fs, inode->singly_indir_block_ptr, temp_blk_buf);
        kfree(temp_blk_buf);

        if (alloc_cnt) (*alloc_cnt)++;

        singly_ptr_minted = true;
      }

      uint32_t *indir_blk = kmalloc(BLOCK_SIZE);

      read_block(fs, inode->singly_indir_block_ptr, (uint16_t *) indir_blk);
      if (indir_blk[indir_relative_idx] == 0) {
        if (mode != ALLOC_MODE){
          kfree(indir_blk);
          if (mode == LOOKUP_MODE || mode == FREE_MODE)
            return EMPTY_BLOCK;
          return INODE_ERROR;
        }
        indir_blk[indir_relative_idx] = alloc_block(fs);
        if (!indir_blk[indir_relative_idx]){
          if (singly_ptr_minted) {
            free_block(fs, inode->singly_indir_block_ptr);
            inode->singly_indir_block_ptr = 0;
            if (alloc_cnt) (*alloc_cnt)--;
          }
          kfree(indir_blk);
          return BLOCK_ERROR;
        }
        
        write_block(fs, inode->singly_indir_block_ptr, (uint16_t *) indir_blk);
        if (alloc_cnt) (*alloc_cnt)++;
      }

      *disk_blk_out = indir_blk[indir_relative_idx];
      
      if (mode == FREE_MODE){
        indir_blk[indir_relative_idx] = 0;
        if (alloc_cnt) (*alloc_cnt)++;

        bool elements_inside = false;
        for (uint32_t i = 0; i < BIT_32_PER_BLK; i++) {
          if (indir_blk[i] != 0){
            elements_inside = true;
            break;
          }
        }

        if (!elements_inside) {
          free_block(fs, inode->singly_indir_block_ptr);
          inode->singly_indir_block_ptr = 0;
          if (alloc_cnt) (*alloc_cnt)++;
        }
        else {
          write_block(fs, inode->singly_indir_block_ptr, (uint16_t *) indir_blk);
        }
      }

      kfree(indir_blk);

      return INODE_WRITE_SUCCESS;
    }

    indir_relative_idx -= BIT_32_PER_BLK;

    if (indir_relative_idx < BIT_32_PER_BLK * BIT_32_PER_BLK){
      bool doubly_ptr_minted = false;
      if (inode->doubly_indir_block_ptr == 0) {
        if (mode != ALLOC_MODE){
          if (mode == LOOKUP_MODE || mode == FREE_MODE)
            return EMPTY_BLOCK;
          return INODE_ERROR;
        }
        inode->doubly_indir_block_ptr = alloc_block(fs);
        if (!inode->doubly_indir_block_ptr)
          return BLOCK_ERROR;

        uint16_t *temp_blk_buf = kmalloc(BLOCK_SIZE);
        zero_block_buf((uint32_t *) temp_blk_buf);
        write_block(fs, inode->doubly_indir_block_ptr, temp_blk_buf);
        kfree(temp_blk_buf);
        if (alloc_cnt) (*alloc_cnt)++;
        doubly_ptr_minted = true;
      }
      
      bool singly_ptr_minted = false;
      uint32_t *doubly_indir_blk = kmalloc(BLOCK_SIZE); 
      read_block(fs, inode->doubly_indir_block_ptr, (uint16_t *) doubly_indir_blk);
      if (doubly_indir_blk[indir_relative_idx / BIT_32_PER_BLK] == 0) { 
        if (mode != ALLOC_MODE){
          kfree(doubly_indir_blk);
          if (mode == LOOKUP_MODE || mode == FREE_MODE)
            return EMPTY_BLOCK;
          return INODE_ERROR;
        }
        doubly_indir_blk[indir_relative_idx / BIT_32_PER_BLK] = alloc_block(fs);
        if (!doubly_indir_blk[indir_relative_idx / BIT_32_PER_BLK]){ 
          if (doubly_ptr_minted) {
            free_block(fs, inode->doubly_indir_block_ptr);
            inode->doubly_indir_block_ptr = 0;
            if (alloc_cnt) (*alloc_cnt)--;
          }
          kfree(doubly_indir_blk);
          return BLOCK_ERROR;
        }

        uint16_t *temp_blk_buf = kmalloc(BLOCK_SIZE);
        zero_block_buf((uint32_t *) temp_blk_buf);
        write_block(fs, doubly_indir_blk[indir_relative_idx / BIT_32_PER_BLK], temp_blk_buf);
        kfree(temp_blk_buf);
        write_block(fs, inode->doubly_indir_block_ptr, (uint16_t *) doubly_indir_blk);
        if (alloc_cnt) (*alloc_cnt)++;
        singly_ptr_minted = true;
      } 

      uint32_t *indir_blk = kmalloc(BLOCK_SIZE);
      read_block(fs, doubly_indir_blk[indir_relative_idx / BIT_32_PER_BLK], (uint16_t *) indir_blk);
      if (indir_blk[indir_relative_idx % BIT_32_PER_BLK] == 0) {
        if (mode != ALLOC_MODE){
          kfree(indir_blk);
          kfree(doubly_indir_blk);
          if (mode == LOOKUP_MODE || mode == FREE_MODE)
            return EMPTY_BLOCK;
          return INODE_ERROR;
        }
        indir_blk[indir_relative_idx % BIT_32_PER_BLK] = alloc_block(fs);
        if (!indir_blk[indir_relative_idx % BIT_32_PER_BLK]){
          if (singly_ptr_minted) {
            free_block(fs, doubly_indir_blk[indir_relative_idx / BIT_32_PER_BLK]);
            doubly_indir_blk[indir_relative_idx / BIT_32_PER_BLK] = 0;
            if (alloc_cnt) (*alloc_cnt)--;

            if (doubly_ptr_minted) {
              free_block(fs, inode->doubly_indir_block_ptr);
              inode->doubly_indir_block_ptr = 0;
              if (alloc_cnt) (*alloc_cnt)--;
            }
            else {
              write_block(fs, inode->doubly_indir_block_ptr, (uint16_t *) doubly_indir_blk);
            }
          }
          kfree(indir_blk);
          kfree(doubly_indir_blk);
          return BLOCK_ERROR;
        }
        write_block(fs, doubly_indir_blk[indir_relative_idx / BIT_32_PER_BLK], (uint16_t *) indir_blk);
        if (alloc_cnt) (*alloc_cnt)++;
      }

      *disk_blk_out = indir_blk[indir_relative_idx % BIT_32_PER_BLK];
      
      if (mode == FREE_MODE){
        indir_blk[indir_relative_idx % BIT_32_PER_BLK] = 0;
        if (alloc_cnt) (*alloc_cnt)++;

        bool elements_inside = false;
        for (uint32_t i = 0; i < BIT_32_PER_BLK; i++) {
          if (indir_blk[i] != 0) {
            elements_inside = true;
            break;
          }
        }

        if (!elements_inside) {
          free_block(fs, doubly_indir_blk[indir_relative_idx / BIT_32_PER_BLK]);
          doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK] = 0;
          if (alloc_cnt) (*alloc_cnt)++;

          for (uint32_t i = 0; i < BIT_32_PER_BLK; i++) {
            if (doubly_indir_blk[i] != 0){
              elements_inside = true;
              break;
            }
          }

          if (!elements_inside) {
            free_block(fs, inode->doubly_indir_block_ptr);
            inode->doubly_indir_block_ptr = 0;
            if (alloc_cnt) (*alloc_cnt)++;
          }
          else {
            write_block(fs, inode->doubly_indir_block_ptr, (uint16_t *) doubly_indir_blk);
          }
        }
        else {
          write_block(fs, doubly_indir_blk[indir_relative_idx / BIT_32_PER_BLK], (uint16_t *) indir_blk);
        }
      }
      
      kfree(indir_blk);
      kfree(doubly_indir_blk);

      return INODE_WRITE_SUCCESS;
    }

    indir_relative_idx -= BIT_32_PER_BLK * BIT_32_PER_BLK;

    if (true){
      bool triply_ptr_minted = false;
      if (inode->triply_indir_block_ptr == 0) {
        if (mode != ALLOC_MODE){
          if (mode == LOOKUP_MODE || mode == FREE_MODE)
            return EMPTY_BLOCK;
          return INODE_ERROR;
        }
        inode->triply_indir_block_ptr = alloc_block(fs);
        if (!inode->triply_indir_block_ptr)
          return BLOCK_ERROR;

        uint16_t *temp_blk_buf = kmalloc(BLOCK_SIZE);
        zero_block_buf((uint32_t *) temp_blk_buf);
        write_block(fs, inode->triply_indir_block_ptr, temp_blk_buf);
        kfree(temp_blk_buf);
        if (alloc_cnt) (*alloc_cnt)++;
        triply_ptr_minted = true;
      }
      
      bool doubly_ptr_minted = false;
      uint32_t *triply_indir_blk = kmalloc(BLOCK_SIZE);
      read_block(fs, inode->triply_indir_block_ptr, (uint16_t *) triply_indir_blk);
      if (triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)] == 0) { 
        if (mode != ALLOC_MODE){
          kfree(triply_indir_blk);
          if (mode == LOOKUP_MODE || mode == FREE_MODE)
            return EMPTY_BLOCK;
          return INODE_ERROR;
        }
        triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)] = alloc_block(fs);
        if (!triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)]){ 
          if (triply_ptr_minted) {
            free_block(fs, inode->triply_indir_block_ptr);
            inode->triply_indir_block_ptr = 0;
            if (alloc_cnt) (*alloc_cnt)--;
          }
          kfree(triply_indir_blk);
          return BLOCK_ERROR;
        }

        uint16_t *temp_blk_buf = kmalloc(BLOCK_SIZE);
        zero_block_buf((uint32_t *) temp_blk_buf);
        write_block(fs, triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)], temp_blk_buf);
        kfree(temp_blk_buf);
        write_block(fs, inode->triply_indir_block_ptr, (uint16_t *) triply_indir_blk);
        if (alloc_cnt) (*alloc_cnt)++;
        doubly_ptr_minted = true;
      } 
      
      bool singly_ptr_minted = false;
      uint32_t *doubly_indir_blk = kmalloc(BLOCK_SIZE);
      read_block(fs, triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)], (uint16_t *) doubly_indir_blk);

      if (doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK] == 0) { 
        if (mode != ALLOC_MODE){
          kfree(doubly_indir_blk);
          kfree(triply_indir_blk);
          if (mode == LOOKUP_MODE || mode == FREE_MODE)
            return EMPTY_BLOCK;
          return INODE_ERROR;
        }
        doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK] = alloc_block(fs);
        if (!doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK]){ 
          if (doubly_ptr_minted) {
            free_block(fs, triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)]);
            triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)] = 0;
            if (alloc_cnt) (*alloc_cnt)--;

            if (triply_ptr_minted) {
              free_block(fs, inode->triply_indir_block_ptr);
              inode->triply_indir_block_ptr = 0;
              if (alloc_cnt) (*alloc_cnt)--;
            }
            else {
              write_block(fs, inode->triply_indir_block_ptr, (uint16_t *) triply_indir_blk);
            }
          }
          kfree(doubly_indir_blk);
          kfree(triply_indir_blk);
          return BLOCK_ERROR;
        }

        uint16_t *temp_blk_buf = kmalloc(BLOCK_SIZE);
        zero_block_buf((uint32_t *) temp_blk_buf);
        write_block(fs, doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK], temp_blk_buf);
        kfree(temp_blk_buf);
        write_block(fs, triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)], (uint16_t *) doubly_indir_blk);
        if (alloc_cnt) (*alloc_cnt)++;
        singly_ptr_minted = true;
      } 

      uint32_t *indir_blk = kmalloc(BLOCK_SIZE);
      read_block(fs, doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK], (uint16_t *) indir_blk);
      if (indir_blk[indir_relative_idx % BIT_32_PER_BLK] == 0) { 
        if (mode != ALLOC_MODE){
          kfree(doubly_indir_blk);
          kfree(triply_indir_blk);
          kfree(indir_blk);
          if (mode == LOOKUP_MODE || mode == FREE_MODE)
            return EMPTY_BLOCK;
          return INODE_ERROR;
        }
        indir_blk[indir_relative_idx % BIT_32_PER_BLK] = alloc_block(fs);
        if (!indir_blk[indir_relative_idx % BIT_32_PER_BLK]){
          if (singly_ptr_minted) {
            free_block(fs, doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK]);
            doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK] = 0;
            if (alloc_cnt) (*alloc_cnt)--;

            if (doubly_ptr_minted) {
              free_block(fs, triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)]);
              triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)] = 0;
              if (alloc_cnt) (*alloc_cnt)--;

              if (triply_ptr_minted) {
                free_block(fs, inode->triply_indir_block_ptr);
                inode->triply_indir_block_ptr = 0;
                if (alloc_cnt) (*alloc_cnt)--;
              }
              else {
                write_block(fs, inode->triply_indir_block_ptr, (uint16_t *) triply_indir_blk);
              }
            }
            else {
              write_block(fs, triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)], (uint16_t *) doubly_indir_blk);
            }
          }
          kfree(indir_blk);
          kfree(doubly_indir_blk);
          kfree(triply_indir_blk);
          return BLOCK_ERROR;
        }
        write_block(fs, doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK], (uint16_t *) indir_blk);
        if (alloc_cnt) (*alloc_cnt)++;
      }

      *disk_blk_out = indir_blk[indir_relative_idx % BIT_32_PER_BLK];
      
      if (mode == FREE_MODE){
        indir_blk[indir_relative_idx % BIT_32_PER_BLK] = 0;
        if (alloc_cnt) (*alloc_cnt)++;

        bool elements_inside = false;
        for (uint32_t i = 0; i < BIT_32_PER_BLK; i++) {
          if (indir_blk[i] != 0) {
            elements_inside = true;
            break;
          }
        }

        if (!elements_inside) {
          free_block(fs, doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK]);
          doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK] = 0;
          if (alloc_cnt) (*alloc_cnt)++;

          for (uint32_t i = 0; i < BIT_32_PER_BLK; i++) {
            if (doubly_indir_blk[i] != 0){
              elements_inside = true;
              break;
            }
          }

          if (!elements_inside) {
            free_block(fs, triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)]);
            triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)] = 0;
            if (alloc_cnt) (*alloc_cnt)++;

            for (uint32_t i = 0; i < BIT_32_PER_BLK; i++) {
              if (triply_indir_blk[i] != 0) {
                elements_inside = true;
                break;
              }
            }

            if (!elements_inside) {
              free_block(fs, inode->triply_indir_block_ptr);
              inode->triply_indir_block_ptr = 0;
              if (alloc_cnt) (*alloc_cnt)++;
            }
            else {
              write_block(fs, inode->triply_indir_block_ptr, (uint16_t *) triply_indir_blk);
            }
          }
          else {
            write_block(fs, triply_indir_blk[indir_relative_idx / (BIT_32_PER_BLK * BIT_32_PER_BLK)], (uint16_t *) doubly_indir_blk);
          }
        }
        else {
          write_block(fs, doubly_indir_blk[(indir_relative_idx / BIT_32_PER_BLK) % BIT_32_PER_BLK], (uint16_t *) indir_blk);
        }
      }

      kfree(indir_blk);
      kfree(doubly_indir_blk);
      kfree(triply_indir_blk);
    }
  }
  return INODE_WRITE_SUCCESS;
}

uint32_t append_to_inode(struct fs_info *fs, uint32_t inode_n, uint16_t *buf, uint32_t f_size, struct ext2_inode *out)
{
  struct ext2_inode inode;
  if (!get_inode(fs, inode_n, &inode))
    return INODE_ERROR;
  
  uint32_t old_size = inode.size_lo;
  
  uint32_t size_needed = inode.size_lo + f_size;

  uint32_t old_blk_cnt = (old_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

  uint32_t buf_byte_cursor = 0;

  uint32_t bytes_left = f_size;

  uint32_t alloc_cnt = 0;

  bool alloced_backward = false;


  if (size_needed < old_size) // out of inode space
    return INODE_ERROR;

  if (old_size % BLOCK_SIZE != 0){
    uint16_t *slack_buf = kmalloc(BLOCK_SIZE);
    uint32_t tail_blk_n;
    uint32_t map_result = map_file_block(fs, &inode, old_blk_cnt - 1, LOOKUP_MODE, &tail_blk_n, NULL);
    if (map_result == EMPTY_BLOCK){
      uint32_t alloc_result = map_file_block(fs, &inode, old_blk_cnt - 1, ALLOC_MODE, &tail_blk_n, &alloc_cnt);
      if (alloc_result != INODE_WRITE_SUCCESS) {
        kfree(slack_buf);
        return INODE_ERROR;
      }

      for (uint32_t i = 0; i < WORDS_PER_BLK; i++) {
        slack_buf[i] = 0;
      }

      alloced_backward = true;
    }
    else if (map_result == INODE_WRITE_SUCCESS) {
      read_block(fs, tail_blk_n, slack_buf);
    }
    else {
      kfree(slack_buf);
      return INODE_ERROR;
    }

    uint8_t *slack_bytes =  (uint8_t *) slack_buf;
    uint8_t *src_bytes = (uint8_t *) buf;

    uint32_t tail_off = old_size % BLOCK_SIZE;
    uint32_t splice_len = min(bytes_left, BLOCK_SIZE - tail_off);

    for (uint32_t i = 0; i < splice_len; i++) {
      slack_bytes[tail_off + i] = src_bytes[buf_byte_cursor + i];
    }
    
    buf_byte_cursor += splice_len;
    bytes_left -= splice_len;

    write_block(fs, tail_blk_n, slack_buf);

    kfree(slack_buf);
  }

  uint32_t blk_idx = 0;

  while(bytes_left > 0) {
    uint32_t curr_blk_n;
    if (map_file_block(fs, &inode, old_blk_cnt + blk_idx, ALLOC_MODE, &curr_blk_n, &alloc_cnt) != INODE_WRITE_SUCCESS){
      uint32_t unwind_blk_n;
      uint32_t island_start = old_blk_cnt;
      for (uint32_t i = island_start; i < island_start + blk_idx; i++) { 
        if (map_file_block(fs, &inode, i, FREE_MODE, &unwind_blk_n, NULL) != INODE_WRITE_SUCCESS) {
          panic("KERNEL PANIC: Inode free block operation failed when attempting to free block after append failure, disk corruption occurred!!");
        }
        
        free_block(fs, unwind_blk_n);
      }

      if (alloced_backward) {
        if (map_file_block(fs, &inode, old_blk_cnt - 1, FREE_MODE, &unwind_blk_n, NULL) != INODE_WRITE_SUCCESS) {
          panic("KERNEL PANIC: Inode free block operation failed when attempting to free block after append failure, disk corruption occurred!!");
        }
        
        free_block(fs, unwind_blk_n);
      }

      return INODE_ERROR;
    }
    

    uint16_t *curr_blk_buf = kmalloc(BLOCK_SIZE);

    uint8_t *curr_blk_bytes = (uint8_t *) curr_blk_buf;
    uint8_t *src_bytes = (uint8_t *) buf;
    uint32_t add_amt = min(bytes_left, BLOCK_SIZE);

    for (uint32_t i = 0; i < add_amt; i++) {
      curr_blk_bytes[i] = src_bytes[buf_byte_cursor + i];
    }

    buf_byte_cursor += add_amt;
    bytes_left -= add_amt;

    write_block(fs, curr_blk_n, curr_blk_buf);

    kfree(curr_blk_buf);

    blk_idx++;
  }

  inode.size_lo += f_size;
  inode.disk_sectors += alloc_cnt * sectors_per_blk;
  inode.last_mod_time = timer_get_epoch_sec();

  if (!set_inode(fs, inode_n, &inode)) {
    return INODE_ERROR;
  }

  *out = inode;
  return INODE_WRITE_SUCCESS;
}

void unwind_insert(struct fs_info *fs, uint32_t inode_n, struct ext2_inode *inode, uint32_t *unwind_buf, uint32_t unwind_cursor, uint32_t free_cnt, uint32_t minted_cnt)
{
  uint32_t free_blk_n;
  uint32_t u_cnt = 0;
  for (uint32_t i = 0; i < unwind_cursor; i++) {
    if (map_file_block(fs, inode, unwind_buf[i], FREE_MODE, &free_blk_n, &u_cnt) != INODE_WRITE_SUCCESS) {
      panic("KERNEL PANIC: Attempted to recover data after a failed insert, but map_file_block() FREE_MODE failed!!");
    }
    free_block(fs, free_blk_n);
  }

  kfree(unwind_buf);

  inode->disk_sectors -= (free_cnt + (u_cnt - unwind_cursor - minted_cnt)) * sectors_per_blk;

  if (!set_inode(fs, inode_n, inode))
    panic("KERNEL PANIC: Attempted to recover data after a failed insert, but set_inode() failed!!");
}

uint32_t insert_in_inode(struct fs_info *fs, uint32_t inode_n, uint16_t *buf, uint32_t f_size, uint32_t offset, struct ext2_inode *out) {
  if (f_size == 0) 
    return INODE_WRITE_SUCCESS;
  
  struct ext2_inode inode;
  if (!get_inode(fs, inode_n, &inode))
    return INODE_ERROR;

  uint32_t old_size = inode.size_lo;

  if (offset + f_size < offset || old_size + f_size < old_size)
    return INODE_ERROR;
  
  uint32_t new_size = max(offset + f_size, old_size + f_size);

  uint32_t alloc_cnt = 0;
  uint32_t free_cnt = 0;


  uint32_t top = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE - 1;
  uint32_t bottom = (offset + f_size) / BLOCK_SIZE;

  uint32_t dest_blk = top;

  uint32_t *unwind_buf = kmalloc((top - offset/BLOCK_SIZE + 1) * sizeof(uint32_t));
  uint32_t unwind_cursor = 0;
  uint32_t minted_cnt = 0;

  uint32_t rem = f_size % BLOCK_SIZE;
  uint32_t shift_blks = f_size / BLOCK_SIZE;

  bool the_sky_is_blue = true;
  while(the_sky_is_blue) {
    if (dest_blk < bottom || (dest_blk == bottom && (offset + f_size) % BLOCK_SIZE != 0)) break;
    
    uint32_t src_blk = dest_blk - shift_blks;

    uint32_t curr_upper_blk_n;
    uint32_t map_result = map_file_block(fs, &inode, src_blk, LOOKUP_MODE, &curr_upper_blk_n, NULL);

    if (map_result != INODE_WRITE_SUCCESS && map_result != EMPTY_BLOCK){
      unwind_insert(fs, inode_n, &inode, unwind_buf, unwind_cursor, free_cnt, minted_cnt);
      return map_result;
    }


    uint32_t curr_lower_blk_n;
    uint32_t map_result_lo;
  
    if (rem != 0){
      map_result_lo = map_file_block(fs, &inode, src_blk - 1, LOOKUP_MODE, &curr_lower_blk_n, NULL);
      if (map_result_lo != INODE_WRITE_SUCCESS && map_result_lo != EMPTY_BLOCK){
        unwind_insert(fs, inode_n, &inode, unwind_buf, unwind_cursor, free_cnt, minted_cnt);
        return map_result_lo;
      }
    }

    bool lo_is_hole = (rem == 0) ? true : (map_result_lo == EMPTY_BLOCK);
    bool hi_is_hole = (map_result == EMPTY_BLOCK);


    if (lo_is_hole && hi_is_hole) {
      uint32_t old_blk;
      if(map_file_block(fs, &inode, dest_blk, FREE_MODE, &old_blk, &free_cnt) == INODE_WRITE_SUCCESS){
        free_block(fs, old_blk);
      }
    }

    else{
      uint32_t new_blk_n;
      uint32_t alloc_cnt_snapshot = alloc_cnt;
      uint32_t upd_result = map_file_block(fs, &inode, dest_blk, ALLOC_MODE, &new_blk_n, &alloc_cnt);

      if (upd_result != INODE_WRITE_SUCCESS){
        unwind_insert(fs, inode_n, &inode, unwind_buf, unwind_cursor, free_cnt, minted_cnt);
        return upd_result;
      }

      if (alloc_cnt_snapshot != alloc_cnt) {
        unwind_buf[unwind_cursor++] = dest_blk;
        minted_cnt += (alloc_cnt - alloc_cnt_snapshot) - 1;
      }

      uint8_t *new_blk = kmalloc(BLOCK_SIZE);
      uint32_t blk_idx = 0;
      
      if (!lo_is_hole){
        uint8_t *blk_lo = kmalloc(BLOCK_SIZE);
        read_block(fs, curr_lower_blk_n, (uint16_t *) blk_lo);
        for (uint32_t lower_idx = BLOCK_SIZE - rem; lower_idx < BLOCK_SIZE; lower_idx++){
          new_blk[blk_idx] = blk_lo[lower_idx];
          blk_idx++;
        }
        kfree(blk_lo);
      }
      else{
        for(uint32_t i = 0; i < rem; i++)
          new_blk[blk_idx++] = 0;
      }
      
      if (!hi_is_hole){
        uint8_t *blk_hi = kmalloc(BLOCK_SIZE);
        read_block(fs, curr_upper_blk_n, (uint16_t *) blk_hi); 
        for (uint32_t higher_idx = 0; higher_idx < BLOCK_SIZE - rem; higher_idx++){
          new_blk[blk_idx] = blk_hi[higher_idx];
          blk_idx++;
        }
        kfree(blk_hi);
      }
      else{
        for (uint32_t i = 0; i < BLOCK_SIZE - rem; i++)
          new_blk[blk_idx++] = 0;
      }

      write_block(fs, new_blk_n, (uint16_t *) new_blk);
      kfree(new_blk);
    }

    dest_blk--;
  }

  uint32_t split = (offset + f_size) % BLOCK_SIZE;

  if (split != 0) {
    uint32_t partial_dest_blk;
    uint32_t alloc_cnt_snapshot = alloc_cnt;
    uint32_t partial_map_status = map_file_block(fs, &inode, bottom, ALLOC_MODE, &partial_dest_blk, &alloc_cnt);
    if (partial_map_status != INODE_WRITE_SUCCESS){
      unwind_insert(fs, inode_n, &inode, unwind_buf, unwind_cursor, free_cnt, minted_cnt);
      return INODE_ERROR;
    }

    if (alloc_cnt_snapshot != alloc_cnt) {
      unwind_buf[unwind_cursor++] = bottom;
      minted_cnt += (alloc_cnt - alloc_cnt_snapshot) - 1;
    }
    
    uint8_t *blk = kmalloc(BLOCK_SIZE);
    
    read_block(fs, partial_dest_blk, (uint16_t *) blk);

    uint32_t needed = BLOCK_SIZE - split;
    uint32_t src_byte = offset;
    uint32_t buf_idx = split;

    while (needed > 0) {
      uint32_t src_blk = src_byte / BLOCK_SIZE;
      uint32_t src_offset = src_byte % BLOCK_SIZE;
      uint32_t chunk = min(needed, BLOCK_SIZE - src_offset);

      uint32_t src_blk_n;
      uint32_t map_status = map_file_block(fs, &inode, src_blk, LOOKUP_MODE, &src_blk_n, NULL);
      if (map_status == EMPTY_BLOCK) {
        for (uint32_t i = 0; i < chunk; i ++)
          blk[buf_idx++] = 0;
      }
      else if (map_status == INODE_WRITE_SUCCESS){
        uint8_t *src = kmalloc(BLOCK_SIZE);
        read_block(fs, src_blk_n, (uint16_t *) src);
        for (uint32_t i = 0; i < chunk; i++)
          blk[buf_idx++] = src[src_offset + i];
        kfree(src);
      }
      else{
        kfree(blk);
        unwind_insert(fs, inode_n, &inode, unwind_buf, unwind_cursor, free_cnt, minted_cnt);
        return map_status;
      }

      src_byte += chunk;
      needed -= chunk;
    }

    write_block(fs, partial_dest_blk, (uint16_t *) blk);
    kfree(blk);
  }
  
  uint8_t *buf_bytes = (uint8_t *) buf;
  uint32_t buf_idx = 0;
  uint32_t remaining = f_size;
  uint32_t dest_byte = offset;

  while (remaining > 0) {
    uint32_t blk_idx = dest_byte / BLOCK_SIZE;
    uint32_t blk_offset = dest_byte % BLOCK_SIZE;
    uint32_t chunk = min(remaining, BLOCK_SIZE - blk_offset);

    uint32_t blk_n;
    uint32_t alloc_cnt_snapshot = alloc_cnt;
    uint32_t map_status = map_file_block(fs, &inode, blk_idx, ALLOC_MODE, &blk_n, &alloc_cnt);
    if (map_status != INODE_WRITE_SUCCESS){
      unwind_insert(fs, inode_n, &inode, unwind_buf, unwind_cursor, free_cnt, minted_cnt);
      return map_status;
    }

    if (alloc_cnt_snapshot != alloc_cnt) {
      unwind_buf[unwind_cursor++] = blk_idx;
      minted_cnt += (alloc_cnt - alloc_cnt_snapshot) - 1;
    }

    uint8_t *blk = kmalloc(BLOCK_SIZE);
    if (blk_offset != 0 || chunk != BLOCK_SIZE)
      read_block(fs, blk_n, (uint16_t *) blk);

    if (dest_byte == offset && (dest_byte - blk_offset) >= old_size) {
      uint32_t block_start = dest_byte - blk_offset;
      uint32_t zero_from = (old_size > block_start) ? (old_size - block_start) : 0;
      for (uint32_t i = zero_from; i < blk_offset; i++)
        blk[i] = 0;
    }

    for (uint32_t i = 0; i < chunk; i++) {
      blk[blk_offset + i] = buf_bytes[buf_idx + i];
    }

    write_block(fs, blk_n, (uint16_t *) blk);
    kfree(blk);

    dest_byte += chunk;
    buf_idx += chunk;
    remaining -= chunk;
  }
  
  inode.size_lo = new_size;
  if (alloc_cnt >= free_cnt) 
    inode.disk_sectors += (alloc_cnt - free_cnt) * sectors_per_blk;
  else 
    inode.disk_sectors -= (free_cnt - alloc_cnt) * sectors_per_blk;
  inode.last_mod_time = timer_get_epoch_sec();
  inode.last_access_time = timer_get_epoch_sec();
  if (!set_inode(fs, inode_n, &inode)){
    kfree(unwind_buf);
    return INODE_ERROR;
  }

  *out = inode;

  kfree(unwind_buf);
  return INODE_WRITE_SUCCESS;
}

// calls both mint_inode() and write_inode()
// and links it to the BGDT's inode table (offset 8)
// and updates a few other things.
uint32_t put_inode(struct fs_info *fs, uint16_t *buf, uint32_t f_size, bool is_dir, struct ext2_inode *out)
{
  uint32_t type;
  if (is_dir) {
    type = INODE_DIR_TYPE;
  }
  else{
    type = INODE_FILE_TYPE;
  }

  struct ext2_inode inode;
  uint32_t inode_n = mint_inode(fs, f_size, type, &inode);

  if (inode_n == INODE_ERROR) return INODE_ERROR;

  if (write_inode(fs, buf, &inode) != INODE_WRITE_SUCCESS) {
    free_inode(fs, inode_n);
    return INODE_ERROR;
  }

  uint32_t group_n = (inode_n - first_inode_n) / fs->inodes_per_grp;

  if (group_n >= fs->ngroups || inode_n > fs->superblk->inode_cnt) {
    panic("mint_inode() failed to return a correct value!! disk corruption likely :-(");
  }

  *out = inode;
  
  if (!set_inode(fs, inode_n, &inode)){
    return INODE_ERROR;
  }

  if (is_dir){
    fs->bgdt[group_n].dir_cnt++;
    write_block(fs, bgdt_blk_n, (uint16_t *) fs->bgdt);
  }

  return inode_n;
}

uint32_t truncate_inode(struct fs_info *fs, uint32_t inode_n, uint32_t new_size, struct ext2_inode *out)
{
  struct ext2_inode inode;
  if (!get_inode(fs, inode_n, &inode))
    return INODE_ERROR;

  if (inode.size_lo < new_size) {
    return INODE_ERROR;
  }

  uint32_t remove_start = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
  uint32_t total_blocks = (inode.size_lo + BLOCK_SIZE - 1) / BLOCK_SIZE;
  uint32_t curr_blk_n;
  uint32_t free_cnt = 0;

  for (uint32_t blk = remove_start; blk < total_blocks; blk++) {
    uint32_t map_result = map_file_block(fs, &inode, blk, FREE_MODE, &curr_blk_n, &free_cnt);
    if (map_result != INODE_WRITE_SUCCESS && map_result != EMPTY_BLOCK) {
      return INODE_ERROR;
    }
    if (map_result == EMPTY_BLOCK)
      continue;

    free_block(fs, curr_blk_n);
  }
  
  if (new_size % BLOCK_SIZE != 0){
    uint32_t remainder_rel_blk = new_size / BLOCK_SIZE;
    uint32_t remainder_blk;
    uint32_t map_result = map_file_block(fs, &inode, remainder_rel_blk, LOOKUP_MODE, &remainder_blk, NULL);
    
    if (map_result != INODE_WRITE_SUCCESS && map_result !=  EMPTY_BLOCK) {
      return INODE_ERROR;
    }

    if (map_result != EMPTY_BLOCK) {
      uint32_t start = new_size % BLOCK_SIZE;
      uint8_t *byte_buf = kmalloc(BLOCK_SIZE);

      read_block(fs, remainder_blk, (uint16_t *) byte_buf);

      for (uint32_t byte = start; byte < BLOCK_SIZE; byte++) {
        byte_buf[byte] = 0;
      }

      write_block(fs, remainder_blk, (uint16_t *) byte_buf);

      kfree(byte_buf);
    }
  }

  inode.disk_sectors -= free_cnt * sectors_per_blk;
  inode.size_lo = new_size;
  inode.last_mod_time = timer_get_epoch_sec();
  inode.last_access_time = timer_get_epoch_sec();
  if (!set_inode(fs, inode_n, &inode)){
    return INODE_ERROR;
  }

  *out = inode;

  return INODE_WRITE_SUCCESS;
}

// NOTE: All the functions below are no longer used, kept just-in-case something
// comes up in the future where these should be used

// reads the inputted inode
// self explanatory imo 
__attribute__((unused))
static void read_inode(struct fs_info *fs, struct ext2_inode *inode, uint16_t *out)
{
  uint16_t (*indirect_buf)[WORDS_PER_BLK] = kmalloc(INDIRECT_PTR_LAYERS * BLOCK_SIZE);
  uint32_t blocks_left = (inode->size_lo + BLOCK_SIZE - 1) / BLOCK_SIZE;

  for (int i = 0; i < INODE_BLK_PTR_AMT && blocks_left > 0; i++) {
    indir_read_block(fs, &out, &blocks_left, inode->dir_block_ptr[i], 0, indirect_buf);
  }
  
  if (blocks_left > 0) indir_read_block(fs, &out, &blocks_left, inode->singly_indir_block_ptr, 1, indirect_buf);
  if (blocks_left > 0) indir_read_block(fs, &out, &blocks_left,inode->doubly_indir_block_ptr, 2, indirect_buf);
  if (blocks_left > 0) indir_read_block(fs, &out, &blocks_left, inode->triply_indir_block_ptr, 3, indirect_buf); 
  kfree(indirect_buf);
}
