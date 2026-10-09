#include "app/depsh/cmds/echo.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "errors.h"

#define ECHO_OUT_FLAG " >> "
#define ECHO_OUT_FLAG_LEN 4

void cmd_echo(char *args)
{
  uint32_t len = 0;
  while (args[len] != '\0')
    len++;

  uint32_t dir_path_start = len;
  uint32_t curr_flag_idx = 0;

  if (args[0] == '>' && args[1] == '>' && args[2] == ' ')
    dir_path_start = 3;

  for (uint32_t i = 0; i < len; i++) {
    if (curr_flag_idx >= ECHO_OUT_FLAG_LEN) {
      dir_path_start = i;
    }
    if (ECHO_OUT_FLAG[curr_flag_idx] == args[i]){
      curr_flag_idx++;
    }
    else{
      curr_flag_idx = (args[i] == ' ') ? 1 : 0;
    }
  }
  
  if (dir_path_start < len){
    char path[len - dir_path_start + 1];
    uint32_t path_idx = 0;

    for (uint32_t i = dir_path_start; i < len; i++) {
      path[path_idx++] = args[i];
    }

    path[len - dir_path_start] = '\0';

    const char *absolute_path = return_path(path);

    if (absolute_path == 0){
      write_string("echo: invalid path\n");
      return;
    }

    uint32_t inode_n = resolve_dir(absolute_path);

    if (inode_n == SYSCALL_ERROR) {
      uint32_t parent_n;
      char *leaf;

      if (!split_parent_leaf((char *) absolute_path, &parent_n, &leaf)) {
        write_string("echo: when attempting to create missing file, an unexpected error occurred when splitting the path\n");
        return;
      }

      if ((inode_n = touch(parent_n, (const char *) leaf)) == SYSCALL_ERROR) {
        write_string("echo: when attempting to create missing file, an unexpected error occurred when creating the file\n");
        return;
      }
    }

    if (is_dir(inode_n)) {
      write_string("echo: cannot echo into a directory\n");
      return;
    }

    if (fappend(inode_n, (uint16_t *) args, dir_path_start == 3 ? 0 : dir_path_start - 4) == SYSCALL_ERROR) {
      write_string("echo: disk error or out of space, disk corruption possible\n");
      return;
    }
    if (fappend(inode_n, (uint16_t *) "\n", 1) == SYSCALL_ERROR) {
      write_string("echo: disk error or out of space, disk corruption possible\n");
      return;
    }
  }
  else{
    write_string(args);
    write_string("\n");
  }
}
