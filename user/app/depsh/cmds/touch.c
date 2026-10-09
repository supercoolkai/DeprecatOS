#include "app/depsh/cmds/touch.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "errors.h"
#include <stdint.h>

void cmd_touch(char *args)
{
  if (args[0] == '-') {
    write_string("\ntouch: option ");
    write_string(args);
    write_string(" does not exist\n");
    return;
  }

  char *path = (char *) return_path(args);

  if (path == 0) {
    write_string("touch: current path is too long to fully parse\n");
    return;
  }

  if (path[0] == 0) {
    write_string("touch: no path provided\n");
    return;
  }

  if (resolve_dir((const char *)path) != SYSCALL_ERROR){
    write_string("touch: file already exists\n");
    return;
  }

  char *leaf;
  uint32_t parent_n;

  if (!split_parent_leaf(path, &parent_n, &leaf)) {
    write_string("touch: given parent directory does not exist\n");
    return;
  }

  if (touch(parent_n, leaf) == SYSCALL_ERROR) {
    write_string("touch: failed (out of space or disk error)\n");
    return;
  }
}
