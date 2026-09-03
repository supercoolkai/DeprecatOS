#ifndef BLOCKGROUPDESCRIPTOR_H
#define BLOCKGROUPDESCRIPTOR_H

#include <stdint.h>
#include <stddef.h>

#define BLOCK_SIZE 4096

struct ext2_block_group_descriptor{
  uint32_t block_bitmap_addr;
  uint32_t inode_bitmap_addr;
  uint32_t inode_start_addr;
  uint16_t unalloc_block_cnt;
  uint16_t unalloc_inode_cnt;
  uint16_t dir_cnt;

  uint8_t unused_trail[14];
} __attribute__((packed));

_Static_assert(sizeof(struct ext2_block_group_descriptor) == 32, "size assert");



#endif
