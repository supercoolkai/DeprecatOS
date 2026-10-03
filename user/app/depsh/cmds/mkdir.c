#include "app/depsh/cmds/mkdir.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "errors.h"
#include <stdbool.h>
#include <stdint.h>

static bool mkdir_recursive(char *path)
{
  int last = -1;

  for (int i = 0; path[i] != 0; i++) {
    if (path[i] == '/') last = i;
  }

  char *leaf = path + last + 1;

  char temp = path[last];

  path[last] = 0;

  char *parent = (last == 0) ? "/" : path;

  uint32_t parent_n = resolve_dir((const char *) parent);

  if (parent_n == SYSCALL_ERROR){
    if (!mkdir_recursive(parent)) {
      path[last] = temp;
      return false;
    }
  }

  parent_n = resolve_dir((const char *) parent);
  path[last] = temp;

  if (parent_n == SYSCALL_ERROR)
    return false;


  if (mkdir(parent_n, leaf) == SYSCALL_ERROR){
    return false;
  }

  return true;
}

void cmd_mkdir(char *args)
{
  char *flag = "";
  char *path_arg = (char *) args;

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

  if (path == 0) {
    write_string("mkdir: current path is too long to fully parse\n");
    return;
  }

  if (path[0] == 0) {
    write_string("mkdir: no path provided\n");
    return;
  }

  if (resolve_dir((const char *)path) != SYSCALL_ERROR){
    write_string("mkdir: cannot create directory, already exists\n");
    return;
  }

  int len = 0;
  while (path[len] != 0) len++;

  while (len > 1 && path[len - 1] == '/')
    path[--len] = 0;

  int last = -1;

  for (int i = 0; path[i] != 0; i++) {
    if (path[i] == '/') last = i;
  }

  char *leaf = path + last + 1;

  path[last] = 0;

  char *parent = (last == 0) ? "/" : path;

  uint32_t parent_n = resolve_dir((const char *) parent);

  if (parent_n == SYSCALL_ERROR)
  {
    if (!streq(flag, "-p")) {
      write_string("mkdir: given parent directory does not exist\n");
      return;
    }
    if (!mkdir_recursive(parent)) {
      write_string("mkdir: failed (out of space or disk error)\n");
      return;
    }
    parent_n = resolve_dir((const char *) parent);
  }

  if (mkdir(parent_n, leaf) == SYSCALL_ERROR) {
    write_string("mkdir: failed (out of space or disk error)\n");
    return;
  }
}
