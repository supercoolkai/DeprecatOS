#include "fs/skeleton/skeleton.h"
#include "fs/ext2/directoryController.h"
#include "fs/block/blockController.h"
#include "exceptions/exceptions.h"
#include "memory/heap/kernelHeap.h"
#include "errors.h"
#include <stdint.h>
#include <stdbool.h>

// NOTE: requires u to have a leading slash because its an absolute path
static const char *skeleton_dirs[] = {
  "/etc/journal", // NOTE: hint hint
  "/mnt",
};

static const uint32_t skeleton_dirs_len = sizeof(skeleton_dirs) / sizeof(skeleton_dirs[0]);

static bool make_path(char *path)
{
  int last = -1;

  for (int i = 0; path[i] != 0; i++) {
    if (path[i] == '/') last = i;
  }

  char *leaf = path + last + 1;

  char temp = path[last];

  path[last] = 0;

  char *parent = (last == 0) ? "/" : path;

  struct fs_info *parent_fs;
  uint32_t parent_n;

  if (!lookup_path(root_fs, parent, &parent_n, true, &parent_fs)){
    if (!make_path(parent)) {
      path[last] = temp;
      return false;
    }
  }

  bool resolved = lookup_path(root_fs, parent, &parent_n, true, &parent_fs);
  path[last] = temp;

  if (!resolved)
    return false;


  struct fs_info *throwaway_fs;
  if (make_dir(parent_fs, parent_n, leaf, &throwaway_fs) == INODE_ERROR){
    return false;
  }

  return true;
}

void skeleton_init(void)
{
  uint32_t found_n;
  struct fs_info *throwaway_fs;
  struct ext2_inode found_inode;
  for (uint32_t i = 0; i < skeleton_dirs_len; i++) {
    if (!lookup_path(root_fs, skeleton_dirs[i], &found_n, false, &throwaway_fs)
      || !get_inode(root_fs, found_n, &found_inode)
      || (found_inode.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE){
      
      char *path_buf = kmalloc(NAME_LEN);
      for (uint32_t c = 0; c < NAME_LEN; c++){
        path_buf[c] = skeleton_dirs[i][c];

        if (!path_buf[c])
          break;
      }

      if(!make_path(path_buf)){
        kfree(path_buf);
        panic("KERNEL PANIC: when trying to create the skeleton, make_dir() failed somehow (or a file is in the way. remove it from another OS as it's likely necessary for the OS to run)");
      }

      kfree(path_buf);
    }
  }
}
