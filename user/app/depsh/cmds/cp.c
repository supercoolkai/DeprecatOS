#include "app/depsh/cmds/cp.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "errors.h"
#include <stdint.h>

static bool copy_file(uint32_t src_n, uint32_t inode_n)
{
  uint32_t n = 0;
  uint32_t r;
  for (;;) {
    r = read_chunk(src_n, n, fs_buf);
    if (r == 0){
      return true;
    }

    if (r == SYSCALL_ERROR) {
      write_string("cp: an unknown error occurred while reading the src file, file corruption occured. recommended you delete the copy.\n");

      return false;
    }

    else {
      if (fappend(inode_n, (uint16_t *) fs_buf, r) == SYSCALL_ERROR) {
        write_string("cp: an unknown error occured while writing to the new file, file corruption occured. recommended you delete the copy.\n");
        return false;
      }

      n++;
    }
  }
}

static bool cp_walk(uint32_t src_dir, uint32_t target_dir)
{ 
  char child_name[256];

  uint32_t chunk = 0;
  uint32_t pos = 0;

  for (;;) {
    bool found = false;
    uint32_t child_n = 0;
    bool child_is_dir = false;
    
    
    while (!found){
      uint32_t r = read_chunk(src_dir, chunk, fs_buf);

      if (r == 0) return true;

      if (r == SYSCALL_ERROR) {
        write_string("cp: unknown read error\n");
        return false;
      }

      char *blk_bytes = (char *) fs_buf;
      while (pos < r) {
        uint32_t inode = *(uint32_t *)(blk_bytes + pos);
        uint16_t advance_amt = *(uint16_t *)(blk_bytes + pos + 4);
        uint8_t name_len = blk_bytes[pos + 6];
        uint8_t type = blk_bytes[pos + 7];

        if (advance_amt < 8 || 8 + name_len > advance_amt || pos + advance_amt > r) {
          return false;
        }

        bool is_dot=  (name_len == 1 && blk_bytes[pos+8] == '.');
        bool is_dot2 = (name_len == 2 && blk_bytes[pos+8] == '.' && blk_bytes[pos+9] == '.');

        if (inode != 0 && !is_dot && !is_dot2) {
          for (int i = 0; i < name_len; i++){
            child_name[i] = blk_bytes[pos + 8 + i];
          }
          child_name[name_len] = 0;
          child_n = inode;
          child_is_dir = (type == 2);

          found = true;
          pos += advance_amt;
          break;
        }

        pos += advance_amt;
      }

      if (!found || pos >= r){
        chunk++;
        pos = 0;
      }
    }
    
    if (!child_is_dir) {
      uint32_t new_n = touch(target_dir, child_name);
      if (new_n == SYSCALL_ERROR) {
        write_string("cp: make file failed, disk error or file out of space\n");
        return false;
      }
      if (!copy_file(child_n, new_n))
        return false;
    }
    else{
      uint32_t new_n = mkdir(target_dir, child_name);
      if (new_n == SYSCALL_ERROR) {
        write_string("cp: make dir failed, disk error or file out of space\n");
        return false;
      }
      if (!cp_walk(child_n, new_n))
        return false;
    }
  }

  return false; // NOTE: unreachable but just to get the compiler to stop complaining
}

void cmd_cp(char *args)
{
  char *flag = "";
  char *other_args = args;

  if (args[0] == '-'){
    flag = args;
    int i = 0;
    while (args[i] != ' ' && args[i] != 0)
      i++;

    if (args[i] == ' ') {
      args[i] = 0;
      other_args = args + i + 1;
    }
    else {
      other_args = args + i;
    }
  }
  
  char *old_path = other_args;
  char *new_path = other_args;
  int i = 0;
  while (other_args[i] != ' ' && other_args[i] != 0)
    i++;

  if (other_args[i] == ' ') {
    other_args[i] = 0;
    new_path = other_args + i + 1;
  }
  else {
    new_path = other_args + i;
  }

  if (streq(new_path, "") || streq(old_path, "")){
    write_string("cp: invalid arguments.\n");
    return;
  }

  bool recursive = false;

  if (streq(flag, "-r")) {
    recursive = true;
  }
  
  char *absolute_new_path = (char *) return_path(new_path);

  if (absolute_new_path == 0){
    write_string("cp: path too long\n");
    return;
  }

  if (resolve_dir(absolute_new_path) != SYSCALL_ERROR){
    write_string("cp: new path already taken\n");
    return;
  }
  
  char absolute_new_path_buf[256];
  for (uint32_t i = 0; i < 256; i++) {
    absolute_new_path_buf[i] = absolute_new_path[i];
    if (absolute_new_path[i] == 0)
      break;
  }

  absolute_new_path = absolute_new_path_buf;

  char *absolute_old_path = (char *) return_path(old_path);

  if (absolute_old_path == 0){
    write_string("cp: path too long\n");
    return;
  }

  if (resolve_dir(absolute_old_path) == SYSCALL_ERROR){
    write_string("cp: cannot copy a nonexistent file\n");
    return;
  }

  uint32_t old_len = 0;
  while (absolute_old_path[old_len] != 0) old_len++;

  while (old_len > 1 && absolute_old_path[old_len - 1] == '/')
    old_len--;

  bool inside = true;
  for (uint32_t i = 0; i < old_len; i++) {
    if (absolute_new_path[i] != absolute_old_path[i]) {
      inside = false;
      break;
    }
  }

  if (inside && (old_len == 1 || absolute_new_path[old_len] == '/' || absolute_new_path[old_len] == 0)) {
    write_string("cp: new path cannot be inside the old path\n");
    return;
  }

  uint32_t len = 0;
  while (absolute_new_path[len] != 0) len++;

  while (len > 1 && absolute_new_path[len - 1] == '/')
    absolute_new_path[--len] = 0;

  uint32_t last = -1;

  for (int i = 0; absolute_new_path[i] != 0; i++) {
    if (absolute_new_path[i] == '/') last = i;
  }

  char *new_leaf = absolute_new_path + last + 1;

  absolute_new_path[last] = 0;

  char *new_parent = (last == 0) ? "/" : absolute_new_path;

  uint32_t new_parent_n = resolve_dir((const char *) new_parent);

  if (new_parent_n == SYSCALL_ERROR) {
    write_string("cp: given new parent directory does not exist\n");
    return;
  }
  
  uint32_t src_n = resolve_dir(absolute_old_path); 
  if (is_dir(src_n)) {
    if (!recursive){
      write_string("cp: cannot copy a dir without the -r flag\n");
      return;
    }

    uint32_t inode_n = mkdir(new_parent_n, (const char *) new_leaf);
    if (inode_n == SYSCALL_ERROR) {
      write_string("cp: creating the new file failed unexpectedly\n");
      return;
    }

    cp_walk(src_n, inode_n);
    return;
  }

  uint32_t inode_n = touch(new_parent_n, (const char *) new_leaf);
  if (inode_n == SYSCALL_ERROR) {
    write_string("cp: creating the new file failed unexpectedly\n");
    return;
  }
  
  copy_file(src_n, inode_n);
}
