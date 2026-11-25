#include "app/depsh/cmds/lsblk.h"
#include "sys/syscall.h"
#include "errors.h"
#include "app/depsh/depshCommon.h"
#include "fs/ext2/directoryController.h"
#include <stdbool.h>
#include <stdint.h>

#define NAME_COL_WIDTH 19
#define SIZE_COL_WIDTH 6
#define SECTOR_BYTES 512

#define GLYPH_TEE 0xC3
#define GLYPH_CORNER 0xC0
#define GLYPH_DASH 0xC4

static void pad(int32_t n)
{
  while (n-- > 0)
    write_char(' ');
}

static uint32_t format_size(uint64_t sectors, char *out)
{
  static const char units[] = "BKMGT";
  uint64_t tenths = sectors * SECTOR_BYTES * 10;
  uint32_t unit = 0;

  while (unit < 4 && tenths >= 10240) {
    tenths /= 1024;
    unit++;
  }

  uint64_t whole = tenths / 10;
  uint32_t frac = (uint32_t) (tenths % 10);

  char digits[21];
  uint32_t cnt = 0;
  do {
    digits[cnt++] = '0' + (char) (whole % 10);
    whole /= 10;
  } while (whole > 0);

  uint32_t len = 0;
  while (cnt > 0)
    out[len++] = digits[--cnt];

  if (unit != 0 && frac != 0) {
    out[len++] = '.';
    out[len++] = '0' + (char) frac;
  }

  out[len++] = units[unit];
  return len;
}

static bool next_in_drive(uint32_t from, uint8_t drive)
{
  struct fs_row row;

  for (uint32_t j = from + 1; ; j++) {
    if (get_fs_stat(j, &row) == SYSCALL_ERROR)
      return false;

    if (row.sector_count == 0)
      continue;

    return row.drive_identify == drive;
  }
}

void cmd_lsblk(char *args)
{
  if (!streq(args, "")) {
    write_string("\nlsblk: option ");
    write_string(args);
    write_string(" does not exist\n");
    return;
  }

  write_string("NAME");
  pad(NAME_COL_WIDTH - 4);
  pad(SIZE_COL_WIDTH - 4);
  write_string("SIZE TYPE MOUNTPOINT\n");

  struct fs_row row;
  uint32_t i = 0;
  uint8_t curr_drive = 0;
  bool have_drive = false;
  char drive_letter = 'a';

  for (;;) {
    if (get_fs_stat(i, &row) == SYSCALL_ERROR)
      break;

    if (row.sector_count == 0) {
      i++;
      continue;
    }

    if (!have_drive || row.drive_identify != curr_drive) {
      curr_drive = row.drive_identify;
      have_drive = true;

      write_string("hd");
      write_char(drive_letter++);
      pad(NAME_COL_WIDTH - 3 + SIZE_COL_WIDTH);
      write_string(" disk\n");
    }

    bool last = !next_in_drive(i, curr_drive);
    write_char((char) (last ? GLYPH_CORNER : GLYPH_TEE));
    write_char((char) GLYPH_DASH);

    uint32_t label_len = 0;
    while (row.label[label_len] != 0)
      label_len++;

    write_string_len(row.label, label_len);
    pad(NAME_COL_WIDTH - 2 - (int32_t) label_len);

    char size[24];
    uint32_t size_len = format_size(row.sector_count, size);
    pad(SIZE_COL_WIDTH - (int32_t) size_len);
    write_string_len(size, size_len);

    write_string(" part ");

    if (row.mountpoint == ROOT_INODE_N) {
      write_string("/");
    }
    else if (row.mountpoint != 0) {
      write_string("/mnt/");
      write_string_len(row.label, label_len);
    }

    write_char('\n');
    i++;
  }
}
