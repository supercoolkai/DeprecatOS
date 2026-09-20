#include "fs/ext2/directoryEntry.h"
#include "fs/ext2/directoryController.h"
#include "fs/ext2/inode.h"
#include "memory/heap/kernelHeap.h"
#include "fs/ext2/blockGroupDescriptor.h"
#include "fs/block/blockController.h"
#include "util/kprintf/kprintf.h"
#include "drivers/timer/timerController.h"
#include <stdint.h>
#include <stdbool.h>

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

static bool lookup(uint16_t *buf, const char *name, uint32_t *out, uint32_t size)
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
  uint16_t *blk_buf = kmalloc(BLOCK_SIZE / 2 * INODE_BLK_PTR_AMT * sizeof(uint16_t));
  if (path[0] != '/'){
    kfree(blk_buf);
    return false;
  }

  int len = 0;

  for (int i = 0; i < NAME_LEN; i++) {
    if (path[i] == 0)
      break;

    len++;
  }

  if (len == NAME_LEN){
    kfree(blk_buf);
    return false;
  }
  
  int ind = 1;
  uint32_t curr_inode_num = ROOT_INODE_N;

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

    if (!success){
      kfree(blk_buf);
      return false;
    }
  
    if ((ino.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE){
      kfree(blk_buf);
      return false;
    }

    read_inode(&ino, blk_buf);
    if (!lookup(blk_buf, name, &curr_inode_num, ino.size_lo)){
      kfree(blk_buf);
      return false;
    }

    ind = j+1;
  }
  *out = curr_inode_num;
  kfree(blk_buf);
  return true;
}

// !!! NOT A WRAPPER OF PUT_INODE() !!!
// Step 2 of the inode writing pipeline,
// kinda like get_inode() and read_inode()
bool dir_insert(uint32_t parent_inode_n, const char *name, uint32_t child_inode_n)
{
  uint32_t len = 0;
  while (len < NAME_LEN && name[len] != 0)
    len++;

  if (len == 0 || len >= NAME_LEN)
    return false;

  struct ext2_inode child_inode;
  if (!get_inode(child_inode_n, &child_inode)) {
    return false;
  }

  struct ext2_inode parent_inode;
  if (!get_inode(parent_inode_n, &parent_inode)) {
    return false;
  }
  
  if ((parent_inode.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE) {
    return false;
  }

  uint32_t needed = ALIGN4(8 + len);
  uint16_t *scratch = kmalloc(BLOCK_SIZE);

  for (uint32_t i = 0; i < parent_inode.size_lo / BLOCK_SIZE && i < INODE_BLK_PTR_AMT; i++) {
    read_block(parent_inode.dir_block_ptr[i], scratch);

    uint32_t cursor = 0;
    while (cursor < BLOCK_SIZE) {
      struct ext2_directory_entry *entry = (struct ext2_directory_entry *)((uint8_t *)scratch + cursor);

      uint32_t true_size = ALIGN4(8 + entry->name_len_lo);

      if (entry->curr_entry_size < 8 || true_size > entry->curr_entry_size || cursor + entry->curr_entry_size > BLOCK_SIZE) {
        kfree(scratch);
        return false;
      } 

      uint32_t slack = entry->curr_entry_size - true_size;

      if (slack >= needed){
        struct ext2_directory_entry *new = (struct ext2_directory_entry *)(((uint8_t *) entry) + true_size);

        entry->curr_entry_size = true_size;
        new->curr_entry_size = slack;
        new->inode = child_inode_n;
        new->name_len_lo = len;
        for (uint32_t j = 0; j < len; j++) {
          new->name[j] = name[j];
        }
        new->type = ((child_inode.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_DIR_TYPE) ? 2 : 1;
        write_block(parent_inode.dir_block_ptr[i], scratch);


        kfree(scratch);

        child_inode.hard_link_cnt++;
        if (!set_inode(child_inode_n, &child_inode))
          return false;

        parent_inode.last_mod_time = timer_get_tick();
        
        if (!set_inode(parent_inode_n, &parent_inode))
          return false;

        return true;
      }
      cursor += entry->curr_entry_size;
    }
  }
  
  kfree(scratch);
  return false;
}
