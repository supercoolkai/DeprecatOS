#ifndef SYSCALL_H
#define SYSCALL_H
#include "userland/syscall/syscallController.h"
#include <stdint.h>

uint32_t write_char(char c);
uint32_t write_string(char *c);
uint32_t get_ticks(uint64_t *tick_out);
uint32_t exit_curr(void);
uint32_t read_char(void);
uint32_t yield(void);
uint32_t write_char_color(char c, unsigned char color);
uint32_t write_string_color(char *c, unsigned char color);
uint32_t read_chunk(uint32_t inode_n, uint32_t chunk_num, uint32_t *buf);
uint32_t resolve_dir(const char *c);
uint32_t write_string_len(const char *buf, uint32_t len);
uint32_t get_stat(uint32_t inode_n, uint32_t *buf);
uint32_t mkdir(uint32_t inode_n, const char *name);
uint32_t rm_inode(uint32_t inode_n, const char *name);
uint32_t rm_dir(uint32_t inode_n, const char *name);
uint32_t clear_screen(void);
uint32_t set_cursor(uint32_t row, uint32_t col);
uint32_t get_screen_dims(uint32_t *row_out, uint32_t *col_out);
uint32_t fsave(uint32_t inode_n, uint16_t *buf, uint32_t f_size);
uint32_t touch(uint32_t inode_n, const char *name);
uint32_t set_cursor_no_upd(uint32_t row, uint32_t col);
uint32_t write_char_no_upd(char c);
uint32_t write_string_no_upd(char *c);
uint32_t write_string_len_no_upd(const char *buf, uint32_t len);
uint32_t set_scrollback(bool v);
uint32_t get_cursor(uint32_t *row, uint32_t *col);
uint32_t get_epoch(uint64_t *epoch_out);
uint32_t fmove(uint32_t old_parent_n, uint32_t new_parent_n, const char *old_name, const char *new_name);
uint32_t fappend(uint32_t inode_n, uint16_t *buf, uint32_t f_size);
uint32_t lmake_hard(uint32_t inode_n, uint32_t parent_n, const char *name);

#endif
