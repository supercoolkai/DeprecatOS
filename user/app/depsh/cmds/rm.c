#include "app/depsh/cmds/rm.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "errors.h"
#include <stdbool.h>
#include <stdint.h>

static bool rm_tree(uint32_t parent_n, const char *name, uint32_t dir_n)
{
  char child_name[256];

  for (;;) {
    bool found = false;
    uint32_t child_n = 0;
    bool child_is_dir = false;


    for (uint32_t chunk = 0; !found; chunk++) {
      uint32_t r = read_chunk(dir_n, chunk, fs_buf);

      if (r == 0) break;

      if (r == SYSCALL_ERROR) {
        write_string("rm: unknown read error\n");
        return false;
      }

      char *blk_bytes = (char *) fs_buf;
      uint32_t pos = 0;
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
          break;
        }
      pos += advance_amt;
      }
     }
    if (!found) break;
    if (child_is_dir) {
      if (!rm_tree(dir_n, child_name, child_n)) return false;
    }
    else{
      if (rm_inode(dir_n, child_name) == SYSCALL_ERROR) return false;
    }
  }

  return rm_dir(parent_n, name) != SYSCALL_ERROR;
}


void cmd_rm(char *args)
{
  char *flag = "";
  char *path_arg = (char *) args;
  bool recursive = false;

  if (args[0] == '-') {
    flag = args;

    int i = 0;
    while (args[i] != ' ' && args[i] != 0)
      i++;

    if (args[i] == ' ') {
      args[i] = 0;
      path_arg = args + i + 1;
    }
    else {
      path_arg  = args + i;
    }
  }

  char *path = (char *) return_path(path_arg);

  if (streq(path, "")){
    write_string("rm: no file/dir provided.\n");
    return;
  }

  uint32_t to_remove_n = resolve_dir(path);
  if (to_remove_n == SYSCALL_ERROR){
    write_string("rm: given path to remove does not exist\n");
    return;
  }

  if (flag[0] == '-' && flag[1] == 'r') {
    recursive = true;
  }

  int last = -1;

  for (int i = 0; path[i] != 0; i++) {
    if (path[i] == '/') last = i;
  }

  char *leaf = path + last + 1;

  path[last] = 0;

  char *parent = (last == 0) ? "/" : path;

  uint32_t parent_n = resolve_dir((const char *) parent);

  if (parent_n == SYSCALL_ERROR) {
    write_string("rm: given parent directory does not exist\n");
    return;
  }

  if (recursive) {
    if (!is_dir(to_remove_n)){
      write_string("rm: cannot recursively remove a file\n");
      return;
    }

    if (!rm_tree(parent_n, (const char *) leaf, to_remove_n)){
      write_string("rm: failed (disk error)\n");
      return;
    }
  }

  else{
    if (is_dir(to_remove_n)){
      write_string("rm: cannot remove, is a directory\n");
      return;
    }

    if (rm_inode(parent_n, (const char *) leaf)){
      write_string("rm: failed (disk error)\n");
      return;
    }
  }
}
