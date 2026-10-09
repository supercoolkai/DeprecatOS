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
  
  char *old_leaf;
  uint32_t old_parent_n;

  if (!split_parent_leaf(absolute_old_path, &old_parent_n, &old_leaf)) {
    write_string("mv: given old parent directory does not exist\n");
    return;
  }

  char *new_leaf;
  uint32_t new_parent_n;

  if (!split_parent_leaf(absolute_new_path, &new_parent_n, &new_leaf)) {
    write_string("mv: given new parent directory does not exist\n");
    return;
  }

  if (fmove(old_parent_n, new_parent_n, (const char *) old_leaf, (const char *) new_leaf) == SYSCALL_ERROR) {
    write_string("mv: failed, disk error\n");
    return;
  }
}
