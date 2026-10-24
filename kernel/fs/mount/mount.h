#ifndef MOUNT_H
#define MOUNT_H

#include "fs/block/blockController.h"
#include <stdint.h>
#include <stdbool.h>

void mount_init(void);
bool return_mnt_root(struct fs_info *fs, uint32_t inode_n, struct fs_info **out_fs, uint32_t *out_inode_n);
bool return_mnt_mountpoint(struct fs_info *fs, uint32_t inode_n, struct fs_info **out_fs, uint32_t *out_inode_n);
bool mount_fs(const char *label);
bool umount_fs(const char *label);

#endif
