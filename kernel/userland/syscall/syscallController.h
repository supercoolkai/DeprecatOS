#ifndef SYSCALLCONTROLLER_H
#define SYSCALLCONTROLLER_H

#define SYSCALL_VECTOR_NUMBER 0x80
#define SENTINEL 0xDDDDDDDD

#include <stdint.h>

// lsblk struct
struct fs_row {
  char label[17];
  uint64_t sector_count;
  uint32_t mountpoint;
  uint8_t drive_identify;
};


void syscall_init(void);
uint32_t syscall_handler(uint32_t esp);

#endif
