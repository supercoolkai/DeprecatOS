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

void mount_init(void)
{
  uint32_t root_slot_n = root_fs - fs_entries;

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
    struct fs_info *mount_fs;
    if (!lookup_path(root_fs, (const char *) mount_path, &mount_inode_n, false, &mount_fs)) {
      kprintf(KPRINTF_YELLOW "attempted to mount a partition to a nonexistent dir\n" KPRINTF_RESET);
      continue;
    }

    if (mount_fs != root_fs) {
      kprintf(KPRINTF_YELLOW "mount dir resolved outside the root fs\n" KPRINTF_RESET);
      continue;
    }
  
    struct ext2_inode mount_inode;
    if (!get_inode(root_fs, mount_inode_n, &mount_inode)) {
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


bool return_mnt_root(struct fs_info *fs, uint32_t inode_n, struct fs_info **out_fs, uint32_t *out_inode_n)
{
  if (fs != root_fs){
    *out_fs = fs;
    *out_inode_n = inode_n;
    return false;
  }

  for (uint32_t part = 0; part < MAX_MOUNTS; part++) {
    if (fs_entries[part].mountpoint == inode_n){
      *out_fs = &fs_entries[part];
      *out_inode_n = ROOT_INODE_N;
      return true;
    }
  }

  *out_fs = fs;
  *out_inode_n = inode_n;
  return false;
}

// NOTE: false return value doesn't necessarily mean failure in this case, just means
// that it didnt return a mountpoint
bool return_mnt_mountpoint(struct fs_info *fs, uint32_t inode_n, struct fs_info **out_fs, uint32_t *out_inode_n)
{
  if (fs == root_fs || inode_n != ROOT_INODE_N){
    *out_fs = fs;
    *out_inode_n = inode_n;
    return false;
  }

  *out_fs = root_fs;
  *out_inode_n = fs->mountpoint;
  return true;
}
