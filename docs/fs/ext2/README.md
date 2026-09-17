# Ext2 Filesystem

## data structure tables / related information

### superblock format

src: `src/fs/ext2/superblock.h`: asserted as 1024 bytes, `packed`. on disk always at byte offset 1024, regardless of fs block size.

base fields are off 0-83, extended fields are offset 84+

| off | field | type | notes |
|---|---|---|---|
| 0 | `inode_cnt` | `uint32_t` | total inodes |
| 4 | `block_cnt` | `uint32_t` | total blocks |
| 8 | `super_blocks` | `uint32_t` | reserved (superuser) blocks |
| 12 | `unalloc_blocks` | `uint32_t` | free blocks |
| 16 | `unalloc_inodes` | `uint32_t` | free inodes |
| 20 | `super_block_num` | `uint32_t` | block holding this superblock |
| 24 | `block_size_adjusted` | `uint32_t` | block size = `1024 << this` |
| 28 | `frag_size_adjusted` | `uint32_t` | frag size = `1024 << this` |
| 32 | `blocks_per_group` | `uint32_t` | |
| 36 | `frags_per_group` | `uint32_t` | |
| 40 | `inodes_per_group` | `uint32_t` | |
| 44 | `last_mnt_time` | `uint32_t` | |
| 48 | `last_write_time` | `uint32_t` | |
| 52 | `mnt_since_check` | `uint16_t` | mounts since last fsck |
| 54 | `mnt_between_check` | `uint16_t` | max mounts between fsck |
| 56 | `ext2_signature` | `uint16_t` | `0xEF53` magic |
| 58 | `fs_state` | `uint16_t` | clean / errors |
| 60 | `error_behavior` | `uint16_t` | |
| 62 | `minor_version` | `uint16_t` | |
| 64 | `last_fsck_time` | `uint32_t` | |
| 68 | `check_interval` | `uint32_t` | |
| 72 | `os_id` | `uint32_t` | |
| 76 | `major_version` | `uint32_t` | ≥1 -> extended fields valid |
| 80 | `reserved_id_user` | `uint16_t` | |
| 82 | `reserved_id_group` | `uint16_t` | |
| 84 | `first_unreserved_inode` | `uint32_t` | first non-reserved inode (root = 2) |
| 88 | `inode_size` | `uint16_t` | on-disk inode stride (256 on modern mke2fs; struct describes first 128 B only) |
| 90 | `curr_block_group` | `uint16_t` | |
| 92 | `opt_features_present` | `uint32_t` | |
| 96 | `req_features_present` | `uint32_t` | bit 1 set -> dir entries carry a type byte (else byte 7 is name_len high) |
| 100 | `features_unsupport_ro` | `uint32_t` | |
| 104 | `fs_id[16]` | `uint8_t[16]` | |
| 120 | `vol_name[16]` | `uint8_t[16]` | |
| 136 | `last_mnt_path[64]` | `uint8_t[64]` | |
| 200 | `compress_algs` | `uint32_t` | |
| 204 | `blocks_prealloc_files` | `uint8_t` | |
| 205 | `blocks_prealloc_dirs` | `uint8_t` | |
| 206 | `reserved` | `uint16_t` | |
| 208 | `journal_id[16]` | `uint8_t[16]` | |
| 224 | `journal_inode` | `uint32_t` | |
| 228 | `journal_device` | `uint32_t` | |
| 232 | `orphan_inode_head` | `uint32_t` | |
| 236 | `unused_trail[788]` | `uint8_t[788]` | pad to 1024 |

leave in file cuz it needs these to compile and/or run properly:
 - `offsetof(ext2_signature)==56`
 - `offsetof(first_unreserved_inode)==84`
 - `sizeof==1024`

### block group descriptor format

src: `src/fs/ext2/blockGroupDescriptor.h`: asserted as 32 bytes, `packed`. `BLOCK_SIZE` hardcoded `4096`.

| off | field | type | notes |
|---|---|---|---|
| 0 | `block_bitmap_addr` | `uint32_t` | block of the block bitmap |
| 4 | `inode_bitmap_addr` | `uint32_t` | block of the inode bitmap |
| 8 | `inode_start_addr` | `uint32_t` | first block of the inode table |
| 12 | `unalloc_block_cnt` | `uint16_t` | free blocks in group |
| 14 | `unalloc_inode_cnt` | `uint16_t` | free inodes in group |
| 16 | `dir_cnt` | `uint16_t` | directories in group |
| 18 | `unused_trail[14]` | `uint8_t[14]` | pad to 32 |

> block group descriptor table (BGDT) starts in the block following the superblock.
> in this case, block 1.

### inode format

src: `src/fs/ext2/inode.h` asserted as 128 bytes, `packed`. `INODE_BLK_PTR_AMT = 12`.

| off | field | type | notes |
|---|---|---|---|
| 0 | `type_and_perms_lo` | `uint16_t` | type (`&0xF000`) + perms; `0x4000` = dir |
| 2 | `user_id_lo` | `uint16_t` | |
| 4 | `size_lo` | `uint32_t` | file size low 32 bits |
| 8 | `last_access_time` | `uint32_t` | |
| 12 | `creation_time` | `uint32_t` | |
| 16 | `last_mod_time` | `uint32_t` | |
| 20 | `deletion_time` | `uint32_t` | |
| 24 | `group_id` | `uint16_t` | |
| 26 | `hard_link_cnt` | `uint16_t` | |
| 28 | `disk_sectors` | `uint32_t` | 512-byte sectors used |
| 32 | `flags` | `uint32_t` | |
| 36 | `unused` | `uint32_t` | (os-dependent value 1) |
| 40 | `dir_block_ptr[12]` | `uint32_t[12]` | direct block pointers (48 B) |
| 88 | `singly_indir_block_ptr` | `uint32_t` | |
| 92 | `doubly_indir_block_ptr` | `uint32_t` | |
| 96 | `triply_indir_block_ptr` | `uint32_t` | |
| 100 | `generation_num` | `uint32_t` | |
| 104 | `unused_2` | `uint32_t` | (file ACL) |
| 108 | `unused_3` | `uint32_t` | (dir ACL / size high) |
| 112 | `fragment_addr` | `uint32_t` | |
| 116 | `frag_num` | `uint8_t` | |
| 117 | `frag_size` | `uint8_t` | |
| 118 | `unused_4` | `uint16_t` | |
| 120 | `user_id_hi` | `uint16_t` | |
| 122 | `group_id_hi` | `uint16_t` | |
| 124 | `unused_5` | `uint32_t` | |

leave in file cuz it needs these to compile and/or run properly:
 - `offsetof(dir_block_ptr)==40`
 - `#define NO_PERMISSION_MASK 0xF000`
 - `#define INODE_DIR_TYPE 0x4000`

### directory entry format

src: `src/fs/ext2/directoryEntry.h`: `packed`. header asserted as 8 bytes, `name[]` is flexible len

| off | field | type | notes |
|---|---|---|---|
| 0 | `inode` | `uint32_t` | 0 ⇒ unused slot, skip |
| 4 | `curr_entry_size` | `uint16_t` | `rec_len`; advance cursor by this |
| 6 | `name_len_lo` | `uint8_t` | name length low byte |
| 7 | `type_or_name_len_hi` | `uint8_t` | file type if superblock req-features bit 1 set, else name_len high byte |
| 8 | `name[]` | `uint8_t[]` | not NUL-terminated — compare by (len, bytes) |

> tips for navigation: 
>     `rec_len==0` to prevent inf loop
>     `rec_len<8` to check for incomplete entries
>     `rec_len%4` to check for improperly formatted entries
>     `8+name_len>rec_len` check whether `name_len` and `rec_len` is properly set

## structs

### `ext2_block_group_descriptor` in `src/fs/ext2/blockGroupDescriptor.h`:
  contains general information, including the bitmap addresses, the address where the inodes start, the unallocated block count, the unallocated inode count, and the directory count.

### `ext2_inode` in `src/fs/ext2/inode.h`:
  essentially a file, has a block pointers which point to different blocks containing the data that the inode contains. including indirect block pointers it can be up to 4 TiB per inode. each inode has a unique "id" (commonly referred to as inode_n)

### `ext2_directory_entry` in `src/fs/ext2/directoryEntry.h`: 
a wrapper for the ext2_inode struct, containing an inode_n, the size of that inode, the length of its name (the hi byte can sometimes be the type as well), and its name. 

### `ext2_superblock` in `src/fs/ext2/superblock.h`: 
bytes 1023 - 2047 of block 0. contains general information about the filesystem, including inode count, block count, blocks per group, inodes per group, etc.

## overview
controls the `ext2_*` structs and the overall ext2 filesystem 

### `directory`
as the name says, it is the main controller of directories on the kernel side. contains the tools for resolving paths, looking up dirs, etc.
