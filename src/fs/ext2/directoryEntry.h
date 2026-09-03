#ifndef DIRECTORYENTRY_H
#define DIRECTORYENTRY_H

#include <stdint.h>
#include <stddef.h>

struct ext2_directory_entry {
  uint32_t inode;
  uint16_t curr_entry_size;
  uint8_t name_len_lo;
  uint8_t type_or_name_len_hi;

  uint8_t name[];
} __attribute__((packed));

_Static_assert(sizeof(struct ext2_directory_entry) == 8, "header 8 bytes, variable entry size");

#endif
