#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "fs/ext2/directoryController.h"
#include <stdbool.h>
#include <stdint.h>

uint32_t fs_buf[BIT_32_PER_BLK];
uint8_t stat_buf[128];

static char full_path[256];

static bool upd_full_path(void)
{
  for (int i = 0; i < 256; i++) {
    full_path[i] = 0;
  }
  int i = 0;
  while(i < 256){
    char c = dir[i];
    if (c == 0)
      return true;

    full_path[i] = c;

    i++;
  }

  return false;
}

const char *return_path(char *args)
{
  const char *path;

  if (args[0] != '/') {
    bool success = upd_full_path();

    if (!success){
      return 0;
    }

    int i = 0;
    while (full_path[i] != 0)
      i++;

    full_path[i++] = '/';

    int j = 0;
    while (args[j] != 0) {
      if (i + j >= 255) {
        return 0;
      }

      full_path[i + j] = args[j];
      j++;
    }

    full_path[i + j] = 0;

    path = full_path;
  }

  else{
    path = args;
  }

  return path;
}

bool is_dir(uint32_t inode_n)
{
  get_stat(inode_n, (uint32_t *)stat_buf);

  return ((*(uint16_t *) stat_buf & NO_PERMISSION_MASK) == INODE_DIR_TYPE);
}
