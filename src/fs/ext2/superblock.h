#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

#include <stdint.h>
#include <stddef.h>

struct ext2_superblock {
  uint32_t inode_cnt;
  uint32_t block_cnt;
  uint32_t super_blocks;
  uint32_t unalloc_blocks;
  uint32_t unalloc_inodes;
  uint32_t super_block_num;
  uint32_t block_size_adjusted;
  uint32_t frag_size_adjusted;
  uint32_t blocks_per_group;
  uint32_t frags_per_group;
  uint32_t inodes_per_group;
  uint32_t last_mnt_time;
  uint32_t last_write_time;
  uint16_t mnt_since_check;
  uint16_t mnt_between_check;
  uint16_t ext2_signature;
  uint16_t fs_state;
  uint16_t error_behavior;
  uint16_t minor_version;
  uint32_t last_fsck_time;
  uint32_t check_interval;
  uint32_t os_id;
  uint32_t major_version;
  uint16_t reserved_id_user;
  uint16_t reserved_id_group;

  // Rest of these are for only extended superblock
  uint32_t first_unreserved_inode;
  uint16_t inode_size;
  uint16_t curr_block_group;
  uint32_t opt_features_present;
  uint32_t req_features_present;
  uint32_t features_unsupport_ro;
  uint8_t fs_id[16];
  uint8_t vol_name[16];
  uint8_t last_mnt_path[64];
  uint32_t compress_algs;
  uint8_t blocks_prealloc_files;
  uint8_t blocks_prealloc_dirs;
  uint16_t reserved;
  uint8_t journal_id[16];
  uint32_t journal_inode;
  uint32_t journal_device;
  uint32_t orphan_inode_head;
  
  uint8_t unused_trail[788];
} __attribute__((packed));

_Static_assert(offsetof(struct ext2_superblock, ext2_signature) == 56, "signature offset");
_Static_assert(offsetof(struct ext2_superblock, first_unreserved_inode) == 84, "first unreserved inode offset");
_Static_assert(sizeof(struct ext2_superblock) == 1024, "size assert");

#endif
