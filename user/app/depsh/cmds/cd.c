#include "app/depsh/cmds/cd.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "errors.h"
#include <stdint.h>

void cmd_cd(char *args)
{
  char *path = (char *) return_path(args);

  if (path == 0) {
    write_string("cd: current path is too long to fully parse\n");
    return;
  }

  if (args[0] == 0) {
    path = "/";
  }

  uint32_t d = resolve_dir(path);

  if (d == SYSCALL_ERROR)
  {
    write_string("cd: dir not found\n");
    return;
  }

  if (!is_dir(d)) {
    write_string("cd: cannot cd into a file\n");
    return;
  }

  int i = 0;
  int j = 0;
  int to_write = 0;

  while (path[i] != 0) {
    while (path[i] == '/')
      i++;

    j = i;

    while (path[i] != '/' && path[i] != 0)
      i++;

    int len = i - j;

    if (len == 1 && path[j] == '.')
      continue;


    if (len == 2 && path[j] == '.' && path[j+1] == '.'){
      while (to_write > 0 && dir[to_write - 1] != '/')
        to_write--;

      if (to_write > 0)
        to_write --;

      continue;
    }

    dir[to_write] = '/';
    to_write++;

    for (int k = 0; k < len; k++) {
      dir[to_write] = path[j + k];
      to_write++;
    }
  }

  if (to_write == 0){
    dir[to_write++] = '/';
  }

  dir[to_write] = 0;
}
