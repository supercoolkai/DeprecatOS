#include "app/depsh/cmds/ls.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "errors.h"
#include <stdint.h>

#define COLS_PER_ROW_LS 5

void cmd_ls(char *args)
{
  const char *path = return_path(args);

  if (path == 0)
  {
    write_string("ls: current path is too long to fully parse\n");
    return;
  }

  uint32_t inode_n = resolve_dir(path);

  if (inode_n == SYSCALL_ERROR) {
    write_string("ls: directory ");
    write_string(args);
    write_string(" does not exist or is not a directory\n");
    return;
  }

  if (!is_dir(inode_n)) {
    write_string("ls: cannot list a file\n");
    return;
  }

  uint32_t n = 0;
  uint32_t r;
  int writes = 0;
  for (;;) {
    r = read_chunk(inode_n, n, fs_buf);
    if (r == 0){
      write_char('\n');
      return;
    }

    if (r == SYSCALL_ERROR) {
      write_string("ls: an unknown error occurred while reading dir ");
      write_string(args);
      write_string("\n");

      return;
    }

    else {
      char *blk_bytes = (char *) fs_buf;

      uint32_t pos = 0;

      while (pos < r)
      {
        uint32_t inode = *(uint32_t *)(blk_bytes + pos);
        uint16_t advance_amt = *(uint16_t *)(blk_bytes + pos + 4);
        uint8_t name_len = blk_bytes[pos + 6];

        uint8_t name[name_len];

        for (int i = 0; i < name_len; i++) {
          name[i] = blk_bytes[pos + 8 + i];
        }

        if (advance_amt < 8 || 8 + name_len > advance_amt || pos + advance_amt > r) {
          write_string("ls: a file in dir ");
          write_string(args);
          write_string(" was out of range\n");
          return;
        }

        if (inode != 0 && name[0] != '.') {
          write_string_len((const char *) name, name_len);

          write_char(' ');

          if (writes % COLS_PER_ROW_LS == 0 && writes > 0)
            write_char('\n');

          writes++;
        }
        pos += advance_amt;
      }

      n++;
    }


  }
}
