#ifndef DIRECTORYCONTROLLER_H
#define DIRECTORYCONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

#define NO_PERMISSION_MASK 0xF000
#define INODE_DIR_TYPE 0x4000
#define INODE_FILE_TYPE 0x8000
#define NAME_LEN 256

#define ROOT_INODE_N 2

struct dir_row 
{
  uint32_t inode;
  uint8_t name[NAME_LEN];
};

bool ls_dir(uint16_t *buf, struct dir_row *out, uint32_t *len, uint32_t size, uint32_t max);
bool lookup_path(const char *path, uint32_t *out);
bool dir_insert(uint32_t parent_inode_n, const char *name, uint32_t child_inode_n);
bool make_dir(uint32_t parent_inode_n, const char *name);

#endif
