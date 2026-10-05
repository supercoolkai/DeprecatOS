#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <stddef.h>

#define INODE_BLK_PTR_AMT 12

struct ext2_inode {
  uint16_t type_and_perms_lo;
  uint16_t user_id_lo;
  uint32_t size_lo;
  uint32_t last_access_time;
  uint32_t creation_time;
  uint32_t last_mod_time;
  uint32_t deletion_time;
  uint16_t group_id;
  uint16_t hard_link_cnt;
  uint32_t disk_sectors;
  uint32_t flags;
  uint32_t unused;
  uint32_t dir_block_ptr[INODE_BLK_PTR_AMT];
  uint32_t singly_indir_block_ptr;
  uint32_t doubly_indir_block_ptr;
  uint32_t triply_indir_block_ptr;
  uint32_t generation_num;
  uint32_t unused_2;
  uint32_t unused_3;
  uint32_t fragment_addr;

  uint8_t frag_num;
  uint8_t frag_size;
  uint16_t unused_4;
  uint16_t user_id_hi;
  uint16_t group_id_hi;
  uint32_t unused_5;
} __attribute__((packed));

_Static_assert(sizeof(struct ext2_inode) == 128, "size assert");
_Static_assert(offsetof(struct ext2_inode, dir_block_ptr) == 40, "dir_block_ptr offset");

#endif
