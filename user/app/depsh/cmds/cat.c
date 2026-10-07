#include "app/depsh/cmds/cat.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "colors/colors.h"
#include "errors.h"
#include <stdint.h>

void cmd_cat(char *args)
{
  if (streq(args, "")) {
    write_string("cat: must provide a file to read");
    return;
  }

  const char *path = return_path(args);

  if (path == 0)
  {
    write_string("cat: current path is too long to fully parse\n");
    return;
  }

  uint32_t inode_n = resolve_dir(path);

  if (inode_n == SYSCALL_ERROR) {
    write_string("cat: file ");
    write_string(args);
    write_string(" does not exist or is not a file\n");
    return;
  }

  if (is_dir(inode_n)) {
    write_string("cat: cannot read a dir\n");
    return;
  }

  uint32_t n = 0;
  uint32_t r;
  for (;;) {
    r = read_chunk(inode_n, n, fs_buf);
    if (r == 0){
      write_char_color('%', GREEN);
      write_char('\n');
      return;
    }

    if (r == SYSCALL_ERROR) {
      write_string("cat: an unknown error occurred while reading file ");
      write_string(args);
      write_string("\n");

      return;
    }

    else {
      char *blk_bytes = (char *) fs_buf;

      write_string_len(blk_bytes, r);

      n++;
    }
  }
}
