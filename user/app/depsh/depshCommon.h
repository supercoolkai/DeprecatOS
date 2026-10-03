#ifndef DEPSH_COMMON_H
#define DEPSH_COMMON_H

#include "fs/block/blockController.h"
#include <stdbool.h>
#include <stdint.h>

// Shared scratch buffers for the depsh commands.
extern uint32_t fs_buf[BIT_32_PER_BLK];
extern uint8_t stat_buf[128];

// Current working directory. Owned by shell.c (the prompt prints it);
// cmd_cd rewrites it and return_path reads it.
extern char dir[256];

// Utilities owned by shell.c.
int streq(const char *a, const char *b);
void print_uint32(uint32_t n);

// Helpers defined in depshCommon.c.
const char *return_path(char *args);
bool is_dir(uint32_t inode_n);

#endif
