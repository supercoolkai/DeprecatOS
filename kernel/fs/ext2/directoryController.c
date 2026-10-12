#include "fs/ext2/directoryEntry.h"
#include "strlen/strlen.h"
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
#define MAX_SYMLINK_LEN 59
#define SYMLINK_MAX_DEPTH 8
#define PATH_LEN 4096

static uint16_t sectors_per_blk = BLOCK_SIZE / BYTES_PER_SECTOR;

static struct ext2_directory_entry *return_next_dir_entry(uint16_t *buf, uint32_t *pos)
{
  struct ext2_directory_entry *entry = (struct ext2_directory_entry *)(((uint8_t *)buf) + *pos);
  *pos += entry->curr_entry_size;
  return entry;
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


bool lookup_path(const char *path, uint32_t *out, bool follow_leaf_link)
{
  uint16_t *blk_buf = kmalloc(BLOCK_SIZE);
  uint16_t *splice_buf_a = kmalloc(PATH_LEN);
  uint16_t *splice_buf_b = kmalloc(PATH_LEN);
  const char *p = path;
  if (path[0] != '/'){
    kfree(blk_buf);
    kfree(splice_buf_a);
    kfree(splice_buf_b);
    return false;
  }
  
  for (uint32_t splices = 0; splices <= SYMLINK_MAX_DEPTH; splices++){
    bool spliced = false;
    int len = 0;

    for (int i = 0; i < PATH_LEN; i++) {
      if (p[i] == 0)
        break;

      len++;
    }

    if (len == PATH_LEN){
      kfree(blk_buf);
      kfree(splice_buf_a);
      kfree(splice_buf_b);
      return false;
    }
    
    int ind = 1;
    uint32_t curr_inode_num = ROOT_INODE_N;

    while (ind < len){
      // get the current subdir's length
      char c = p[ind];
      int j = ind;
      while (c != '/'){
        if (j >= len){
          j = len+1;
          break;
        }

        c = p[j];
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

      if (subdir_len >= NAME_LEN){
        kfree(blk_buf);
        kfree(splice_buf_a);
        kfree(splice_buf_b);
        return false;
      }

      // get name of subdir now
      
      char name[256];

      for (int i = ind; i < j; i++) {
        name[i-ind] = p[i];
      }

      name[subdir_len] = 0;

      struct ext2_inode ino;    

      bool success = get_inode(curr_inode_num, &ino);

      if (!success){
        kfree(blk_buf);
        kfree(splice_buf_a);
        kfree(splice_buf_b);
        return false;
      }
 
      if ((ino.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE){
        kfree(blk_buf);
        kfree(splice_buf_a);
        kfree(splice_buf_b);
        return false;
      }

      uint32_t blk_cnt = (ino.size_lo + BLOCK_SIZE - 1) / BLOCK_SIZE;
      bool found = false;

      for (uint32_t relative_blk = 0; relative_blk < blk_cnt; relative_blk++) {
        uint32_t dest_blk;
        uint32_t lookup_map_status = map_file_block(&ino, relative_blk, LOOKUP_MODE, &dest_blk, NULL, NULL, NULL);
        if (lookup_map_status != INODE_WRITE_SUCCESS) {
          kfree(blk_buf);
          kfree(splice_buf_a);
          kfree(splice_buf_b);
          return false;
        }

        read_block(dest_blk, blk_buf);

        if (lookup(blk_buf, name, &curr_inode_num, BLOCK_SIZE)) {
          found = true;
          break;
        }
      }

      if (!found) {
        kfree(blk_buf);
        kfree(splice_buf_a);
        kfree(splice_buf_b);
        return false;
      }

      success = get_inode(curr_inode_num, &ino);
      if (!success){
        kfree(blk_buf);
        kfree(splice_buf_a);
        kfree(splice_buf_b);
        return false;
      }
      
      if (j == len && !follow_leaf_link) {
        break;
      }

      if ((ino.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_SYMLINK_TYPE) {
        char *spare;
        if (splices % 2 == 0) {
          // use splice_buf_a
          spare = (char *) splice_buf_a;
        }
        else{
          spare = (char *) splice_buf_b;
        }
        
        char *target;
        uint16_t *buf = NULL;

        if (ino.disk_sectors == 0){
          target = (char *) &ino.dir_block_ptr[0];
        }
        else{
          buf = kmalloc(BLOCK_SIZE);
          read_block(ino.dir_block_ptr[0], buf);

          target = (char *) buf;
        }

        uint32_t tlen = ino.size_lo;

        char *dst = spare;
        uint32_t w = 0;

        if (target[0] != '/') {
          // relative case
          for (uint32_t k = 0; k < (uint32_t) ind; k++) {
            dst[w++] = p[k];
          }
        }

        for (uint32_t k = 0; k < tlen; k++){
          if (w >= PATH_LEN - 1) {
            kfree(blk_buf);
            kfree(splice_buf_a);
            kfree(splice_buf_b);
            kfree(buf);
            return false;
          }
          dst[w++] = target[k];
        }

        for (int k = j; k < len; k ++) {
          if (w >= PATH_LEN - 1){
            kfree(blk_buf);
            kfree(splice_buf_a);
            kfree(splice_buf_b);
            kfree(buf);
            return false;
          }
          dst[w++] = p[k];
        }

        dst[w] = 0;
        p = dst;
        spliced = true;
        kfree(buf);
        break;
      }

      ind = j+1;
    }
    if (spliced)
      continue;
    *out = curr_inode_num;
    kfree(blk_buf);
    kfree(splice_buf_a);
    kfree(splice_buf_b);
    return true;
  }
  kfree(blk_buf);
  kfree(splice_buf_a);
  kfree(splice_buf_b);
  return false;
}

static bool name_in(struct ext2_inode inode, const char *name, uint32_t *out)
{
  uint16_t *scan = kmalloc(BLOCK_SIZE);
  for (uint32_t i = 0; i < inode.size_lo / BLOCK_SIZE; i++) {
    uint32_t dest_blk;
  
    uint32_t lookup_map_status = map_file_block(&inode, i, LOOKUP_MODE, &dest_blk, NULL, NULL, NULL);
    if (lookup_map_status != INODE_WRITE_SUCCESS){
      kfree(scan);
      return false;
    }

    read_block(dest_blk, scan);

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

bool dir_contains(uint32_t parent_inode_n, const char *name, uint32_t *out)
{
  struct ext2_inode parent_inode;
  if (!get_inode(parent_inode_n, &parent_inode))
    return false;

  if ((parent_inode.type_and_perms_lo & NO_PERMISSION_MASK) != INODE_DIR_TYPE)
    return false;

  return name_in(parent_inode, name, out);
}


static bool dir_is_empty(struct ext2_inode inode)
{
  uint16_t *scan = kmalloc(BLOCK_SIZE);
  for (uint32_t i = 0; i < inode.size_lo / BLOCK_SIZE; i++) {
    uint32_t dest_blk;
  
    uint32_t lookup_map_status = map_file_block(&inode, i, LOOKUP_MODE, &dest_blk, NULL, NULL, NULL);
    if (lookup_map_status != INODE_WRITE_SUCCESS){
      kfree(scan);
      return false;
    }

    read_block(dest_blk, scan);

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

  for (uint32_t i = 0; i < parent_inode.size_lo / BLOCK_SIZE; i++) {
    uint32_t dest_blk;
  
    uint32_t lookup_map_status = map_file_block(&parent_inode, i, LOOKUP_MODE, &dest_blk, NULL, NULL, NULL);
    if (lookup_map_status != INODE_WRITE_SUCCESS){
      kfree(scratch);
      return false;
    }

    read_block(dest_blk, scratch);

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
        if ((child_inode.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_SYMLINK_TYPE) {
          entry->type = 7;
        }
        else if ((child_inode.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_DIR_TYPE) {
          entry->type = 2;
        }
        else{
          entry->type = 1;
        }
        write_block(dest_blk, scratch);

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
        if ((child_inode.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_SYMLINK_TYPE) {
          new->type = 7;
        }
        else if ((child_inode.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_DIR_TYPE) {
          new->type = 2;
        }
        else{
          new->type = 1;
        }
        write_block(dest_blk, scratch);


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

  if (((uint64_t) parent_inode.size_lo) + needed > MAX_FILE_SIZE){
    kfree(scratch);
    return false;
  }

  uint32_t alloc_cnt = 0;

  uint32_t dest_blk;
  uint32_t relative_blk = (parent_inode.size_lo + BLOCK_SIZE - 1) / BLOCK_SIZE;

  uint32_t alloc_map_status = map_file_block(&parent_inode, relative_blk, ALLOC_MODE, &dest_blk, &alloc_cnt, NULL, NULL);
  if (alloc_map_status != INODE_WRITE_SUCCESS){
    kfree(scratch);
    return false;
  }
  parent_inode.disk_sectors += alloc_cnt * sectors_per_blk;
  
  uint16_t *buf = kmalloc(BLOCK_SIZE);
  for (uint32_t i = 0; i < WORDS_PER_BLK; i++) {
    buf[i] = 0;
  }
 
  struct ext2_directory_entry *seed = (struct ext2_directory_entry *)buf;
  seed->curr_entry_size = BLOCK_SIZE;
  seed->inode = child_inode_n;

  seed->name_len_lo = len;
  for (uint32_t j = 0; j < len; j++) {
    seed->name[j] = name[j];
  }
  if ((child_inode.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_SYMLINK_TYPE) {
    seed->type = 7;
  }
  else if ((child_inode.type_and_perms_lo & NO_PERMISSION_MASK) == INODE_DIR_TYPE) {
    seed->type = 2;
  }
  else{
    seed->type = 1;
  }

  write_block(dest_blk, buf);

  parent_inode.size_lo += BLOCK_SIZE;
  if (parent_inode_n == child_inode_n) {
    parent_inode.last_mod_time = timer_get_epoch_sec();
    parent_inode.hard_link_cnt++;
    if (!set_inode(parent_inode_n, &parent_inode)) {
      kfree(buf);
      kfree(scratch);
      return false;
    }
  }
  else{
    child_inode.hard_link_cnt++;
    if (!set_inode(child_inode_n, &child_inode)){
      kfree(buf);
      kfree(scratch);
      return false;
    }
    parent_inode.last_mod_time = timer_get_epoch_sec();
    if (!set_inode(parent_inode_n, &parent_inode)){
      kfree(buf);
      kfree(scratch);
      return false;
    }
  }
  

  kfree(buf);
  kfree(scratch);

  return true;
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
  for (uint32_t i = 0; i < parent_inode.size_lo / BLOCK_SIZE; i++) {
    uint32_t dest_blk;
  
    uint32_t lookup_map_status = map_file_block(&parent_inode, i, LOOKUP_MODE, &dest_blk, NULL, NULL, NULL);
    if (lookup_map_status != INODE_WRITE_SUCCESS){
      kfree(scratch);
      return false;
    }
    read_block(dest_blk, scratch);

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
          write_block(dest_blk, scratch);
          kfree(scratch);
          
          *removed_inode_n = inode_n;
        }
        else {
          prev->curr_entry_size += entry->curr_entry_size;

          write_block(dest_blk, scratch);
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
  for (uint32_t i = 0; i < child_inode.size_lo / BLOCK_SIZE; i++) {
    uint32_t dest_blk;
  
    uint32_t lookup_map_status = map_file_block(&child_inode, i, LOOKUP_MODE, &dest_blk, NULL, NULL, NULL);
    if (lookup_map_status != INODE_WRITE_SUCCESS){
      kfree(scratch);
      return false;
    }
    read_block(dest_blk, scratch);

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
        write_block(dest_blk, scratch);
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

bool make_symlink(uint32_t parent_n, const char *name, const char *target)
{
  uint32_t len = strlen(target);
  if (len == 0 || len >= BLOCK_SIZE)
    return false;
  
  struct ext2_inode symlink;
  uint32_t link_n = mint_inode(len, INODE_SYMLINK_TYPE, &symlink);

  if (link_n == INODE_ERROR){
    return false;
  }
  
  uint32_t block_n = 0;

  if (len > MAX_SYMLINK_LEN) {
    // typical write, slower so thats why not default
    uint32_t alloc_map_status = map_file_block(&symlink, 0, ALLOC_MODE, &block_n, NULL, NULL, NULL);

    if (alloc_map_status != INODE_WRITE_SUCCESS){
      free_inode(link_n);
      return false;
    }

    char *buf = kmalloc(BLOCK_SIZE);
    for (uint32_t i = 0; i <= len; i++) {
      buf[i] = target[i];
    }

    write_block(block_n, (uint16_t *) buf);
    kfree(buf);

    symlink.disk_sectors += sectors_per_blk;
  }
  else{
    // interpret the blk ptr field stuff as a 60-byte char buf
    char *inline_target = (char *) &symlink.dir_block_ptr[0];

    for (uint32_t i = 0; i <= len; i++) {
      inline_target[i] = target[i];
    }
  }

  if (!set_inode(link_n, &symlink)){
    free_inode(link_n);
    if (block_n) {
      free_block(block_n);
    }
    return false;
  }
  
  if (!dir_insert(parent_n, name, link_n)){
    free_inode(link_n);
    if (block_n) {
      free_block(block_n);
    }
    return false;
  }

  return true;
}

// NOTE: All the functions below are no longer used, kept just-in-case something
// comes up in the future where these should be used

__attribute__((unused))
static bool ls_dir(uint16_t *buf, struct dir_row *out, uint32_t *len, uint32_t size, uint32_t max)
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
