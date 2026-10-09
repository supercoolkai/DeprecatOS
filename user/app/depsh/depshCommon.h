#ifndef DEPSH_COMMON_H
#define DEPSH_COMMON_H

#include "fs/block/blockController.h"
#include "streq/streq.h"
#include <stdbool.h>
#include <stdint.h>

extern uint32_t fs_buf[BIT_32_PER_BLK];
extern uint8_t stat_buf[128];

extern char dir[256];

void print_uint32(uint32_t n);
void print_uint64(uint64_t n);
void print_uint64_pad(uint64_t n, uint64_t width);

const char *return_path(char *args);
bool is_dir(uint32_t inode_n);
bool split_parent_leaf(char *abs_path, uint32_t *parent_n_out, char **leaf_out);

#endif
