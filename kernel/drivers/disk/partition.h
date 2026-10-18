#ifndef PARTITION_H
#define PARTITION_H

#define MBR_SIZE 512

#include "drivers/disk/ata.h"
#include <stdint.h>

#define DRIVE_UNUSED_SIZE 446
#define PART_ENTRY_SIZE 16
#define PART_GOOD_OFFSET 4
#define PART_LBA_OFFSET 8
#define PART_SECTOR_COUNT_OFFSET 12

struct drive_mbr{
  uint8_t unused[446];
  uint8_t part_entry_1[16];
  uint8_t part_entry_2[16];
  uint8_t part_entry_3[16];
  uint8_t part_entry_4[16];

  uint8_t partition_magic_1;
  uint8_t partition_magic_2;
};

struct drive_mbr *read_mbr(struct ata_drive drive);

#endif
