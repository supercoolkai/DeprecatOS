#ifndef SYSCALL_H
#define SYSCALL_H
#define SENTINEL 0xDDDDDDDD
#include <stdint.h>

uint32_t write_char(char c);
uint32_t write_string(char *c);
uint32_t get_ticks(void);
uint32_t exit_curr(void);
uint32_t read_char(void);
uint32_t yield(void);
uint32_t write_char_color(char c, unsigned char color);
uint32_t write_string_color(char *c, unsigned char color);
uint32_t read_chunk(uint32_t inode_n, uint32_t chunk_num, uint32_t *buf);
uint32_t resolve_dir(const char *c);
uint32_t write_string_len(const char *buf, uint32_t len);
uint32_t get_stat(uint32_t inode_n, uint32_t *buf);

#endif
