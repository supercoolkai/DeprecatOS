#include "fs/ext2/directoryEntry.h"
#include "fs/ext2/directoryController.h"
#include "fs/ext2/inode.h"
#include "fs/ext2/blockGroupDescriptor.h"
#include "fs/block/blockController.h"
#include <stdint.h>
#include <stdbool.h>

static uint16_t buf[BLOCK_SIZE / 2 * INODE_BLK_PTR_AMT];

static struct ext2_directory_entry *return_next_dir_entry(uint16_t *buf, uint32_t *pos)
{
  struct ext2_directory_entry *entry = (struct ext2_directory_entry *)(((uint8_t *)buf) + *pos);
  *pos += entry->curr_entry_size;
  return entry;
}

bool ls_dir(uint16_t *buf, struct dir_row *out, uint32_t *len, uint32_t size, uint32_t max)
{
  uint32_t cursor = 0;
  *len = 0;

  while (cursor < size)
  {
    if (*len == max)
      return false;

    struct ext2_directory_entry *entry = return_next_dir_entry(buf, &cursor);

    if (entry->curr_entry_size == 0)
      return false;

    if (entry->inode == 0)
      continue;
    
    out[*len].inode = entry->inode;
    
    for (int i = 0; i < entry->name_len_lo; i++)
      out[*len].name[i] = entry->name[i];

    out[*len].name[entry->name_len_lo] = 0;

    (*len)++;
  }

  return true;
}

static bool comp_name(const uint8_t *a, uint8_t len, const char *b)
{
  for (int i = 0; i < len; i++) {
    if (a[i] != b[i])
      return false;
  }

  if (b[len] != 0)
    return false;

  return true;
}

bool lookup(uint16_t *buf, const char *name, uint32_t *out, uint32_t size)
{
  uint32_t cursor = 0;
  
  while (cursor < size)
  {
    struct ext2_directory_entry *entry = return_next_dir_entry(buf, &cursor);

    if(entry->curr_entry_size == 0)
      return false;
    if (entry->inode == 0)
      continue;

    if (comp_name(entry->name, entry->name_len_lo, name)){
      *out = entry->inode;
      return true;
    }
  }

  return false;
}


bool lookup_path(const char *path, uint32_t *out)
{
  if (path[0] != '/')
    return false;

  int len = 0;

  for (int i = 0; i < NAME_LEN; i++) {
    if (path[i] == 0)
      break;

    len++;
  }

  if (len == NAME_LEN)
    return false;
  
  int ind = 1;
  uint32_t curr_inode_num = 2;

  while (ind < len){
    // get the current subdir's length
    char c = path[ind];
    int j = ind;
    while (c != '/'){
      if (j >= len){
        j = len+1;
        break;
      }

      c = path[j];
      j++;
    }

    j--;

    if (j < ind)
      j = ind;

    int subdir_len = j - ind;

    if (subdir_len == 0){
      ind = j + 1;
      continue;
    }

    // get name of subdir now
    
    char name[256];

    for (int i = ind; i < j; i++) {
      name[i-ind] = path[i];
    }

    name[subdir_len] = 0;

    struct ext2_inode ino;    

    bool success = get_inode(curr_inode_num, &ino);

    if (!success)
      return false;
  
    if ((ino.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE)
      return false;

    read_inode(&ino, buf);
    if (!lookup(buf, name, &curr_inode_num, ino.size_lo))
      return false;

    ind = j+1;
  }
  *out = curr_inode_num;
  return true;
}
