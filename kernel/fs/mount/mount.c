#include "fs/mount/mount.h"
#include "fs/block/blockController.h"
#include "fs/ext2/superblock.h"
#include "streq/streq.h"
#include "kprintf/kprintf.h"
#include "fs/ext2/directoryController.h"
#include "exceptions/exceptions.h"
#include <stdint.h>
#include <stdbool.h>

#define MOUNT_PATH_SEED "/mnt/"
#define MOUNT_PATH_SEED_LEN 5

static uint32_t root_slot_n = 0xFFFFFFFF;

void mount_init(void)
{
  for (uint32_t i = 0; i < 4; i ++){
    if (fs_entries[i].sector_count != 0) {
      root_slot_n = i;
      break;
    }
  }

  if (root_slot_n == 0xFFFFFFFF){
    panic("KERNEL PANIC: No root partition available");
  }

  char mount_path[MOUNT_PATH_SEED_LEN + FS_INFO_LABEL_LEN];
  for (uint32_t i = root_slot_n + 1; i < MAX_MOUNTS; i++) {
    if (fs_entries[i].sector_count == 0)
      continue;

    struct ext2_superblock *part_superblk = fs_entries[i].superblk;
    
    if(streq((const char *)part_superblk->vol_name, ".") || streq((const char *) part_superblk->vol_name, "..")){
      kprintf(KPRINTF_YELLOW "invalid id for partition\n" KPRINTF_RESET);
      continue;
    }

    bool epic_fail = false;
    bool found_char  = false;
    for (uint32_t j = 0 ;j < 16; j++) {
      if (part_superblk->vol_name[j] == '/'){
        kprintf(KPRINTF_YELLOW "invalid id for partition\n" KPRINTF_RESET);
        epic_fail = true;
        fs_entries[i].label[0] = '\0';
        break;
      }
      if (part_superblk->vol_name[j]) {
        found_char = true;
      }
      fs_entries[i].label[j] = (char) part_superblk->vol_name[j];
    }
    if (epic_fail)
      continue;
    if (!found_char) {
      kprintf(KPRINTF_YELLOW "partition label is blank\n" KPRINTF_RESET);
      continue;
    }

    fs_entries[i].label[FS_INFO_LABEL_LEN - 1] = '\0';
  
    for (uint32_t j = root_slot_n + 1; j < i; j++) {
      if (streq((const char *) fs_entries[i].label, (const char *) fs_entries[j].label)){
        kprintf(KPRINTF_YELLOW "partition has a duplicate label\n" KPRINTF_RESET);
        epic_fail = true;
        break;
      }
    }
    if (epic_fail)
      continue;

    for (uint32_t j = 0; j < MOUNT_PATH_SEED_LEN; j++) {
      mount_path[j] = MOUNT_PATH_SEED[j];
    }
    for (uint32_t j = MOUNT_PATH_SEED_LEN; j < MOUNT_PATH_SEED_LEN + FS_INFO_LABEL_LEN; j++) {
      mount_path[j] = fs_entries[i].label[j - MOUNT_PATH_SEED_LEN];
    }
    uint32_t mount_inode_n;
    if (!lookup_path((const char *) mount_path, &mount_inode_n, false)) {
      kprintf(KPRINTF_YELLOW "attempted to mount a partition to a nonexistent dir\n" KPRINTF_RESET);
      continue;
    }
  
    struct ext2_inode mount_inode;
    if (!get_inode(mount_inode_n, &mount_inode)) {
      kprintf(KPRINTF_YELLOW "bad mount directory\n" KPRINTF_RESET);
      continue;
    }

    if ((mount_inode.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE){
      kprintf(KPRINTF_YELLOW "mount inode is not a directory\n" KPRINTF_RESET);
      continue;
    }

    fs_entries[i].mountpoint = mount_inode_n;
  }
}
