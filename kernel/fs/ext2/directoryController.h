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
bool dir_contains(uint32_t parent_inode_n, const char *name, uint32_t *out);
uint32_t make_dir(uint32_t parent_inode_n, const char *name);
bool dir_remove(uint32_t parent_inode_n, const char *name, uint32_t *removed_inode_n);
bool unlink_inode(uint32_t parent_inode_n, const char *name);
bool unlink_dir(uint32_t parent_inode_n, const char *name);
uint32_t make_file(uint32_t parent_inode_n, const char *name);
bool rename_inode(uint32_t old_parent_n, const char *old_name, uint32_t new_parent_n, const char *new_name);

#endif
