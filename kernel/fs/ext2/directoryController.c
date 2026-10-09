#include "fs/ext2/directoryEntry.h"
#include "fs/ext2/directoryController.h"
#include "fs/ext2/inode.h"
#include "memory/heap/kernelHeap.h"
#include "fs/ext2/blockGroupDescriptor.h"
#include "fs/block/blockController.h"
#include "kprintf/kprintf.h"
#include "drivers/timer/timerController.h"
#include "fs/ext2/bitmapController.h"
#include "errors.h"
#include "streq/streq.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_DIR_DEPTH 1024

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

static bool name_in(struct ext2_inode inode, const char *name, uint32_t *out)
{
  uint16_t *scan = kmalloc(BLOCK_SIZE);
  for (uint32_t i = 0; i < inode.size_lo / BLOCK_SIZE && i < INODE_BLK_PTR_AMT; i++) {
    read_block(inode.dir_block_ptr[i], scan);

    uint32_t cursor = 0;
    while (cursor < BLOCK_SIZE) {
      struct ext2_directory_entry *entry = (struct ext2_directory_entry *)((uint8_t *)scan + cursor);
      uint32_t true_size = ALIGN4(8 + entry->name_len_lo);
      if (entry->curr_entry_size < 8 || true_size > entry->curr_entry_size || cursor + entry->curr_entry_size > BLOCK_SIZE) break;
      if (entry->inode != 0 && comp_name(entry->name, entry->name_len_lo, name)) {
        kfree(scan);
        *out = entry->inode;
        return true;
      }
      cursor += entry->curr_entry_size;
    }
  }

  kfree(scan);
  *out = INODE_ERROR;
  return false;
}


static bool dir_is_empty(struct ext2_inode inode)
{
  uint16_t *scan = kmalloc(BLOCK_SIZE);
  for (uint32_t i = 0; i < inode.size_lo / BLOCK_SIZE && i < INODE_BLK_PTR_AMT; i++) {
    read_block(inode.dir_block_ptr[i], scan);

    uint32_t cursor = 0;
    while (cursor < BLOCK_SIZE) {
      struct ext2_directory_entry *entry = (struct ext2_directory_entry *)((uint8_t *)scan + cursor);
      uint32_t true_size = ALIGN4(8 + entry->name_len_lo);
      if (entry->curr_entry_size < 8 || true_size > entry->curr_entry_size || cursor + entry->curr_entry_size > BLOCK_SIZE) break;
      if (entry->inode != 0 && !comp_name(entry->name, entry->name_len_lo, ".") && !comp_name(entry->name, entry->name_len_lo, "..")) {
        kfree(scan);
        return false;
      }
      cursor += entry->curr_entry_size;
    }
  }

  kfree(scan);
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

      if (entry->inode == 0 && entry->curr_entry_size >= needed) {
        entry->inode = child_inode_n;
        entry->name_len_lo = len;
        for (uint32_t j = 0; j < len; j++) {
          entry->name[j] = name[j];
        }
        entry->type = ((child_inode.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_DIR_TYPE) ? 2 : 1;
        write_block(parent_inode.dir_block_ptr[i], scratch);

        kfree(scratch);

        child_inode.hard_link_cnt++;
        if(parent_inode_n == child_inode_n) {
          child_inode.last_mod_time = timer_get_epoch_sec();

          if (!set_inode(child_inode_n, &child_inode)) return false;
        }
        else{
          if (!set_inode(child_inode_n, &child_inode)) return false;
          parent_inode.last_mod_time = timer_get_epoch_sec();
          if (!set_inode(parent_inode_n, &parent_inode)) return false;
        }

        return true;
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
        if(parent_inode_n == child_inode_n) {
          child_inode.last_mod_time = timer_get_epoch_sec();

          if (!set_inode(child_inode_n, &child_inode)) return false;
        }
        else{
          if (!set_inode(child_inode_n, &child_inode)) return false;
          parent_inode.last_mod_time = timer_get_epoch_sec();
          if (!set_inode(parent_inode_n, &parent_inode)) return false;
        }

        return true;
      }
      cursor += entry->curr_entry_size;
    }
  }
  
  kfree(scratch);
  return false;
}

bool dir_remove(uint32_t parent_inode_n, const char *name, uint32_t *removed_inode_n)
{
  uint32_t len = 0;
  while (len < NAME_LEN && name[len] != 0)
    len++;

  if (len == 0 || len >= NAME_LEN)
    return false;

  struct ext2_inode parent_inode;

  if (!get_inode(parent_inode_n, &parent_inode)) 
    return false;
  
  if ((parent_inode.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE) 
    return false;

  uint16_t *scratch = kmalloc(BLOCK_SIZE);
  for (uint32_t i = 0; i < parent_inode.size_lo / BLOCK_SIZE && i < INODE_BLK_PTR_AMT; i++) {
    read_block(parent_inode.dir_block_ptr[i], scratch);

    uint32_t cursor = 0;
    struct ext2_directory_entry *prev = (struct ext2_directory_entry *)((uint8_t *)scratch);
    while (cursor < BLOCK_SIZE) {
      struct ext2_directory_entry *entry = (struct ext2_directory_entry *)((uint8_t *)scratch + cursor);
      uint32_t true_size = ALIGN4(8 + entry->name_len_lo);
      
      if (entry->curr_entry_size < 8 || true_size > entry->curr_entry_size || cursor + entry->curr_entry_size > BLOCK_SIZE){
        kfree(scratch);
        return false;
      }

      if (entry->inode != 0 && comp_name(entry->name, entry->name_len_lo, name)) {
        uint32_t inode_n = entry->inode;
        if (cursor == 0) {
          entry->inode = 0;
          write_block(parent_inode.dir_block_ptr[i], scratch);
          kfree(scratch);
          
          *removed_inode_n = inode_n;
        }
        else {
          prev->curr_entry_size += entry->curr_entry_size;

          write_block(parent_inode.dir_block_ptr[i], scratch);
          kfree(scratch);
        
          *removed_inode_n = inode_n;
        }
        
        struct ext2_inode inode;
        if (!get_inode(inode_n, &inode)){
          return false;
        }

        inode.hard_link_cnt--;
        if(parent_inode_n == inode_n) {
          inode.last_mod_time = timer_get_epoch_sec();

          if (!set_inode(inode_n, &inode)) {
            return false;
          }
        }
        else{
          if (!set_inode(inode_n, &inode)) {
            return false;
          }
          parent_inode.last_mod_time = timer_get_epoch_sec();
          if (!set_inode(parent_inode_n, &parent_inode)) {
            return false;
          }
        }

        return true;
      }
      cursor += entry->curr_entry_size;

      prev = entry;
    }
  }

  kfree(scratch);


  return false;
}

uint32_t make_dir(uint32_t parent_inode_n, const char *name)
{
  uint32_t len = 0;
  while (len < NAME_LEN && name[len] != 0)
    len++;

  if (len == 0 || len >= NAME_LEN)
    return INODE_ERROR;

  struct ext2_inode parent_inode;
  if (!get_inode(parent_inode_n, &parent_inode))
    return INODE_ERROR;


  if ((parent_inode.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE) {
    kprintf(KPRINTF_RED "make_dir: given parent directory is not a directory\n" KPRINTF_RESET);
    return INODE_ERROR;
  }

  uint32_t temp;
  if (name_in(parent_inode, name, &temp)){
    kprintf(KPRINTF_RED "make_dir: directory already exists in given path\n" KPRINTF_RESET);
    return INODE_ERROR;
  }

  uint16_t *buf = kmalloc(BLOCK_SIZE);
  for (uint32_t i = 0; i < WORDS_PER_BLK; i++) {
    buf[i] = 0;
  }

  struct ext2_directory_entry *seed = (struct ext2_directory_entry *)buf;
  seed->curr_entry_size = BLOCK_SIZE;

  struct ext2_inode ino;
  uint32_t child_n = put_inode(buf, BLOCK_SIZE, true, &ino);

  if (child_n == INODE_ERROR) {
    kfree(buf);
    return INODE_ERROR;
  }

  if(!dir_insert(child_n, ".", child_n)){
    kprintf(KPRINTF_RED "make_dir: dir_insert() failed mid call, disk corruption occurred!\n" KPRINTF_RESET);
    kfree(buf);
    return INODE_ERROR;
  }
  if (!dir_insert(child_n, "..", parent_inode_n)){
    kprintf(KPRINTF_RED "make_dir: dir_insert() failed mid call, disk corruption occurred!\n" KPRINTF_RESET);
    kfree(buf);
    return INODE_ERROR;
  }
  if (!dir_insert(parent_inode_n, name, child_n)){
    kprintf(KPRINTF_RED "make_dir: dir_insert() failed mid call, disk corruption occurred!\n" KPRINTF_RESET);
    kfree(buf);
    return INODE_ERROR;
  }

  kfree(buf);

  return child_n;
}

bool unlink_inode(uint32_t parent_inode_n, const char *name)
{
  struct ext2_inode parent_inode;
  if (!get_inode(parent_inode_n, &parent_inode)) {
    return false;
  }

  if ((parent_inode.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE) {
    kprintf(KPRINTF_RED "unlink_inode: given parent directory is not a directory\n" KPRINTF_RESET);
    return false;
  }

  uint32_t remove_inode_n;

  if (!name_in(parent_inode, name, &remove_inode_n)) {
    kprintf(KPRINTF_RED "unlink_inode: given path does not exist\n" KPRINTF_RESET);
    return false;
  }

  struct ext2_inode to_remove;
  if (!get_inode(remove_inode_n, &to_remove))
    return false;

  if ((to_remove.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_DIR_TYPE) {
    kprintf(KPRINTF_RED "unlink_inode: given path is a directory\n" KPRINTF_RESET);
    return false;
  }

  uint32_t removed;
  if (!dir_remove(parent_inode_n, name, &removed)){
    kprintf(KPRINTF_RED "unlink_inode: dir_remove() failed mid call, disk corruption occurred!\n" KPRINTF_RESET);
    return false;
  }

  struct ext2_inode removed_inode;
  if (!get_inode(removed, &removed_inode)) 
    return false;

  if (removed_inode.hard_link_cnt == 0) {
    if (!delete_inode(removed)){
      kprintf(KPRINTF_RED "unlink_inode: delete_inode() failed mid call, disk corruption occurred!\n" KPRINTF_RESET);
      return false;
    }
  }

  return true;
}

bool unlink_dir(uint32_t parent_inode_n, const char *name)
{
  if (streq(name, ".") || streq(name, "..")) {
    kprintf(KPRINTF_RED "unlink_dir: cannot remove . or ..\n" KPRINTF_RESET);
    return false;
  }

  struct ext2_inode parent_inode;
  if (!get_inode(parent_inode_n, &parent_inode)) {
    return false;
  }

  if ((parent_inode.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE) {
    kprintf(KPRINTF_RED "unlink_dir: given parent directory is not a directory\n" KPRINTF_RESET);
    return false;
  }

  uint32_t remove_inode_n;

  if (!name_in(parent_inode, name, &remove_inode_n)) {
    kprintf(KPRINTF_RED "unlink_dir: given path does not exist\n" KPRINTF_RESET);
    return false;
  }


  struct ext2_inode to_remove;
  if (!get_inode(remove_inode_n, &to_remove))
    return false;

  if ((to_remove.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE) {
    kprintf(KPRINTF_RED "unlink_dir: given path is not a directory\n" KPRINTF_RESET);
    return false;
  }

  if (!dir_is_empty(to_remove)){
    kprintf(KPRINTF_RED "unlink_dir: given path is not empty\n" KPRINTF_RESET);
    return false;
  }

  uint32_t removed;
  if (!dir_remove(parent_inode_n, name, &removed)){
    kprintf(KPRINTF_RED "unlink_dir: dir_remove() failed mid call, disk corruption occurred!\n" KPRINTF_RESET);
    return false;
  }

  if (!get_inode(parent_inode_n, &parent_inode)){
    return false;
  }

  parent_inode.hard_link_cnt--;

  if (!set_inode(parent_inode_n, &parent_inode)) {
    kprintf(KPRINTF_RED "unlink_dir: set_inode() failed mid call, disk corruption occurred!\n" KPRINTF_RESET);
    return false;
  }

  struct ext2_inode removed_inode;
  if (!get_inode(removed, &removed_inode)) 
    return false;

  if (!delete_inode(removed)){
    kprintf(KPRINTF_RED "unlink_dir: delete_inode() failed mid call, disk corruption occurred!\n" KPRINTF_RESET);
    return false;
  }

  return true;
}

uint32_t make_file(uint32_t parent_inode_n, const char *name)
{
  uint32_t len = 0;
  while (len < NAME_LEN && name[len] != 0)
    len++;

  if (len == 0 || len >= NAME_LEN)
    return INODE_ERROR;

  struct ext2_inode parent_inode;
  if (!get_inode(parent_inode_n, &parent_inode))
    return INODE_ERROR;


  if ((parent_inode.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE) {
    kprintf(KPRINTF_RED "make_file: given parent directory is not a directory\n" KPRINTF_RESET);
    return INODE_ERROR;
  }

  uint32_t temp;
  if (name_in(parent_inode, name, &temp)){
    kprintf(KPRINTF_RED "make_file: file already exists in given path\n" KPRINTF_RESET);
    return INODE_ERROR;
  }

  struct ext2_inode inode;
  uint32_t child_inode_n = put_inode(NULL, 0, false, &inode);
  if (child_inode_n == INODE_ERROR)
    return INODE_ERROR;

  if (!dir_insert(parent_inode_n, name, child_inode_n)){
    kprintf(KPRINTF_RED "make_file: dir_insert() failed mid call!\n" KPRINTF_RESET);
    free_inode(child_inode_n);
    return INODE_ERROR;
  }

  return child_inode_n;
}

static bool dir_set_parent_entry(uint32_t child_n, uint32_t new_parent_n)
{
  struct ext2_inode child_inode;
  if (!get_inode(child_n, &child_inode))
    return false;
  
  if ((child_inode.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE) {
    return false;
  }
  

  // NOTE: function does not require parent_inode, but it helps ensure that the inode exists and is a directory
  struct ext2_inode new_parent_inode;
  if (!get_inode(new_parent_n, &new_parent_inode))
    return false;

  if ((new_parent_inode.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE) {
    return false;
  }
   
  uint16_t *scratch = kmalloc(BLOCK_SIZE);
  bool found = false;
  for (uint32_t i = 0; i < child_inode.size_lo / BLOCK_SIZE && i < INODE_BLK_PTR_AMT; i++) {
    read_block(child_inode.dir_block_ptr[i], scratch);

    uint32_t cursor = 0;
    while (cursor < BLOCK_SIZE) {
      struct ext2_directory_entry *entry = (struct ext2_directory_entry *)((uint8_t *)scratch + cursor); 
      uint32_t true_size = ALIGN4(8 + entry->name_len_lo);

      if (entry->curr_entry_size < 8 || true_size > entry->curr_entry_size || cursor + entry->curr_entry_size > BLOCK_SIZE){
        kfree(scratch);
        return false;
      }
      if (entry->inode != 0 && comp_name(entry->name, entry->name_len_lo, "..")) {
        entry->inode = new_parent_n;
        write_block(child_inode.dir_block_ptr[i], scratch);
        found = true;
        break;
      }
      cursor += entry->curr_entry_size;
    }

    if (found)
      break;
  }
  
  kfree(scratch);

  if (!found) {
    return false;
  }


  return true;
}


static bool is_ancestor(uint32_t maybe_ancestor, uint32_t maybe_descendant)
{
  uint32_t cur = maybe_descendant;
  for (uint32_t depth = 0; depth < MAX_DIR_DEPTH; depth++){
    if (cur == maybe_ancestor)
      return true;
    if (cur == ROOT_INODE_N)
      return false;

    struct ext2_inode ino;
    if (!get_inode(cur, &ino))
      return true;
    if (!name_in(ino, "..", &cur))
      return true;
  }

  return true;
}


bool rename_inode(uint32_t old_parent_n, const char *old_name, uint32_t new_parent_n, const char *new_name)
{
  if (streq(old_name, ".") || streq(old_name, "..") || streq(new_name, ".") || streq(new_name, "..")){
    kprintf(KPRINTF_RED "rename_inode: Cannot rename to and from \".\" or \"..\"\n" KPRINTF_RESET);
    return false;
  }
  
  struct ext2_inode old_parent;
  if (!get_inode(old_parent_n, &old_parent)){
    kprintf(KPRINTF_RED "rename_inode: given old parent dir does not exist\n" KPRINTF_RESET);
    return false;
  }
  
  if ((old_parent.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE) {
    kprintf(KPRINTF_RED "rename_inode: given old parent directory is not a directory\n" KPRINTF_RESET);
    return false;
  }

  struct ext2_inode new_parent;
  if (!get_inode(new_parent_n, &new_parent)){
    kprintf(KPRINTF_RED "rename_inode: given new parent dir does not exist\n" KPRINTF_RESET);
    return false;
  }
  
  if ((new_parent.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE) {
    kprintf(KPRINTF_RED "rename_inode: given new parent directory is not a directory\n" KPRINTF_RESET);
    return false;
  }

  uint32_t child_n;
  if (!name_in(old_parent, old_name, &child_n)){
    kprintf(KPRINTF_RED "rename_inode: given child inode is not in the given old parent\n" KPRINTF_RESET);
    return false;
  }
  
  uint32_t throwaway;
  if (name_in(new_parent, new_name, &throwaway)) {
    kprintf(KPRINTF_RED "rename_inode: given new name is already taken in the given new parent\n" KPRINTF_RESET);
    return false;
  }

  if (child_n == ROOT_INODE_N){
    kprintf(KPRINTF_RED "rename_inode: cannot rename the root directory\n" KPRINTF_RESET);
    return false;
  }

  struct ext2_inode child;
  if (!get_inode(child_n, &child)) {
    kprintf(KPRINTF_RED "rename_inode: given child inode does not exist somehow?? no idea how u got this error\n" KPRINTF_RESET);
    return false;
  }

  if ((child.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_DIR_TYPE){
    if (is_ancestor(child_n, new_parent_n)) {
      kprintf(KPRINTF_RED "rename_inode: cannot move a directory into its own child\n" KPRINTF_RESET);
      return false;
    }
  }

  if (!dir_insert(new_parent_n, new_name, child_n)) {
    kprintf(KPRINTF_RED "rename_inode: dir_insert failed in an unexpected way! dont worry this didn't corrupt your disk, but there's a high chance it was corrupted to begin with\n" KPRINTF_RESET);
    return false;
  }
  
  if (!dir_remove(old_parent_n, old_name, &throwaway)){
    kprintf(KPRINTF_RED "rename_inode: dir_remove failed mid call in an unexpected way! your disk has been corrupted!\n" KPRINTF_RESET);
    return false;
  }

  if (old_parent_n != new_parent_n) {
    if ((child.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_DIR_TYPE){
      if (!dir_set_parent_entry(child_n, new_parent_n)){
        kprintf(KPRINTF_RED "rename_inode: dir_set_parent_entry failed mid call in an unexpected way! your disk has been corrupted!\n" KPRINTF_RESET);
        return false;
      } 

      if (!get_inode(old_parent_n, &old_parent)) {
        kprintf(KPRINTF_RED "rename_inode: get_inode failed mid call in an unexpected way! your disk has been corrupted!\n" KPRINTF_RESET);
        return false;
      }

      old_parent.hard_link_cnt--;

      if (!set_inode(old_parent_n, &old_parent)) {
        kprintf(KPRINTF_RED "rename_inode: set_inode failed mid call in an unexpected way! your disk has been corrupted!\n" KPRINTF_RESET);
        return false;
      }


      if (!get_inode(new_parent_n, &new_parent)) {
        kprintf(KPRINTF_RED "rename_inode: get_inode failed mid call in an unexpected way! your disk has been corrupted!\n" KPRINTF_RESET);
        return false;
      }

      new_parent.hard_link_cnt++;

      if (!set_inode(new_parent_n, &new_parent)) {
        kprintf(KPRINTF_RED "rename_inode: set_inode failed mid call in an unexpected way! your disk has been corrupted!\n" KPRINTF_RESET);
        return false;
      }
    }
  }


  return true;
}
