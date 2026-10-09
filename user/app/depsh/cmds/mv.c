#include "app/depsh/cmds/mv.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "errors.h"
#include <stdint.h>

void cmd_mv(char *args)
{
  char *old_path = args;
  char *new_path = args;

  int i = 0;
  while (args[i] != ' ' && args[i] != 0)
    i++;

  if (args[i] == ' ') {
    args[i] = 0;
    new_path = args + i + 1;
  }
  else {
    new_path = args + i;
  }

  if (streq(new_path, "") || streq(old_path, "")){
    write_string("mv: invalid arguments.\n");
    return;
  }

  char *absolute_new_path = (char *) return_path(new_path);

  if (absolute_new_path == 0){
    write_string("mv: path too long\n");
    return;
  }

  if (resolve_dir(absolute_new_path) != SYSCALL_ERROR){
    write_string("mv: new path already taken\n");
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
    write_string("mv: path too long\n");
    return;
  }

  if (resolve_dir(absolute_old_path) == SYSCALL_ERROR){
    write_string("mv: cannot rename a nonexistent file\n");
    return;
  }
  
  int len = 0;
  while (absolute_old_path[len] != 0) len++;

  while (len > 1 && absolute_old_path[len - 1] == '/')
    absolute_old_path[--len] = 0;

  int last = -1;

  for (int i = 0; absolute_old_path[i] != 0; i++) {
    if (absolute_old_path[i] == '/') last = i;
  }

  char *old_leaf = absolute_old_path + last + 1;

  absolute_old_path[last] = 0;

  char *old_parent = (last == 0) ? "/" : absolute_old_path;

  uint32_t old_parent_n = resolve_dir((const char *) old_parent);

  if (old_parent_n == SYSCALL_ERROR) {
    write_string("mv: given old parent directory does not exist\n");
    return;
  }
  
  len = 0;
  while (absolute_new_path[len] != 0) len++;

  while (len > 1 && absolute_new_path[len - 1] == '/')
    absolute_new_path[--len] = 0;

  last = -1;

  for (int i = 0; absolute_new_path[i] != 0; i++) {
    if (absolute_new_path[i] == '/') last = i;
  }

  char *new_leaf = absolute_new_path + last + 1;

  absolute_new_path[last] = 0;

  char *new_parent = (last == 0) ? "/" : absolute_new_path;

  uint32_t new_parent_n = resolve_dir((const char *) new_parent);

  if (new_parent_n == SYSCALL_ERROR) {
    write_string("mv: given new parent directory does not exist\n");
    return;
  }

  if (fmove(old_parent_n, new_parent_n, (const char *) old_leaf, (const char *) new_leaf) == SYSCALL_ERROR) {
    write_string("mv: failed, disk error\n");
    return;
  }
}
