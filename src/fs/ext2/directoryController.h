#ifndef DIRECTORYCONTROLLER_H
#define DIRECTORYCONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

#define NO_PERMISSION_MASK 0xF000
#define INODE_DIR_TYPE 0x4000
#define NAME_LEN 256

struct dir_row 
{
  uint32_t inode;
  uint8_t name[NAME_LEN];
};

bool ls_dir(uint16_t *buf, struct dir_row *out, uint32_t *len, uint32_t size, uint32_t max);
bool lookup(uint16_t *buf, const char *name, uint32_t *out, uint32_t size);
bool lookup_path(const char *path, uint32_t *out);

#endif
