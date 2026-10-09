#include "sys/syscall.h"
#include <stdint.h>

#define SYS_WRITE_CHAR 1
#define SYS_WRITE_STR 2
#define SYS_GET_TICKS 3
#define SYS_EXIT 4
#define SYS_READ_CHAR 5
#define SYS_YIELD 6
#define SYS_WRITE_CHAR_COLOR 7
#define SYS_WRITE_STR_COLOR 8
#define SYS_READ_CHUNK 9
#define SYS_RESOLVE_DIR 10
#define SYS_WRITE_STR_LEN 11
#define SYS_GET_STAT 12
#define SYS_MAKE_DIR 13
#define SYS_REMOVE_INODE 14
#define SYS_REMOVE_DIR 15
#define SYS_GET_SCREEN_DIMS 16
#define SYS_SET_CURSOR 17
#define SYS_CLEAR_SCREEN 18
#define SYS_REPLACE_INODE 19
#define SYS_MAKE_INODE 20
#define SYS_SET_CURSOR_NO_UPD 21
#define SYS_WRITE_CHAR_NO_UPD 22
#define SYS_WRITE_STR_NO_UPD 23
#define SYS_WRITE_STR_LEN_NO_UPD 24
#define SYS_SET_SCROLLBACK 25
#define SYS_GET_CURSOR 26
#define SYS_GET_EPOCH 27
#define SYS_RENAME_INODE 28

uint32_t write_char(char c)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_WRITE_CHAR), "b" ((uint32_t) c)
  );
  return result;
}
uint32_t write_string(char *c)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_WRITE_STR), "b" ((uint32_t) c) : "memory"
  );

  return result;
}
uint32_t get_ticks(uint64_t *tick_out)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_GET_TICKS), "b" ((uint32_t) tick_out) : "memory"
  );

  return result;
}
uint32_t exit_curr(void)
{
 uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_EXIT)
  );

  return result;
}
uint32_t read_char(void)
{
 uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_READ_CHAR)
  );

  return result;
}
uint32_t yield(void)
{
 uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_YIELD)
  );

  return result;
}

uint32_t write_char_color(char c, unsigned char color)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_WRITE_CHAR_COLOR), "b" ((uint32_t) c), "c" ((uint32_t) color)
  );
  return result;
}
uint32_t write_string_color(char *c, unsigned char color)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_WRITE_STR_COLOR), "b" ((uint32_t) c), "c" ((uint32_t) color): "memory"
  );

  return result;
}
uint32_t read_chunk(uint32_t inode_n, uint32_t chunk_num, uint32_t *buf)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_READ_CHUNK), "b" (inode_n), "d" (chunk_num), "c" ((uint32_t) buf): "memory"
  );

  return result;
}
uint32_t resolve_dir(const char *c)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_RESOLVE_DIR), "b" ((uint32_t) c): "memory"
  );

  return result;
}

uint32_t write_string_len(const char *buf, uint32_t len)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_WRITE_STR_LEN), "b" ((uint32_t) buf), "c" (len): "memory"
  );

  return result;
}

uint32_t get_stat(uint32_t inode_n, uint32_t *buf)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_GET_STAT), "b" (inode_n), "c" ((uint32_t) buf): "memory"
  );

  return result;
}

uint32_t mkdir(uint32_t inode_n, const char *name)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_MAKE_DIR), "b" (inode_n), "c" ((uint32_t) name): "memory"
  );

  return result;
}

uint32_t rm_inode(uint32_t inode_n, const char *name)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_REMOVE_INODE), "b" (inode_n), "c" ((uint32_t) name): "memory"
  );

  return result;
}

uint32_t rm_dir(uint32_t inode_n, const char *name)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_REMOVE_DIR), "b" (inode_n), "c" ((uint32_t) name): "memory"
  );

  return result;
}

uint32_t get_screen_dims(uint32_t *row_out, uint32_t *col_out)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_GET_SCREEN_DIMS), "b" (row_out), "c" (col_out): "memory"
  );

  return result;
}

uint32_t set_cursor(uint32_t row, uint32_t col)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_SET_CURSOR), "b" (row), "c" (col): "memory"
  );

  return result;
}

uint32_t clear_screen(void)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_CLEAR_SCREEN)
  );

  return result;
}

uint32_t fsave(uint32_t inode_n, uint16_t *buf, uint32_t f_size)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_REPLACE_INODE), "b" (inode_n), "d" (f_size), "c" ((uint32_t) buf): "memory"
  );

  return result;
}

uint32_t touch(uint32_t inode_n, const char *name)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_MAKE_INODE), "b" (inode_n), "c" ((uint32_t) name): "memory"
  );

  return result;
}

uint32_t set_cursor_no_upd(uint32_t row, uint32_t col)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_SET_CURSOR_NO_UPD), "b" (row), "c" (col): "memory"
  );

  return result;
}

uint32_t write_char_no_upd(char c)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_WRITE_CHAR_NO_UPD), "b" ((uint32_t) c)
  );
  return result;
}

uint32_t write_string_no_upd(char *c)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_WRITE_STR_NO_UPD), "b" ((uint32_t) c) : "memory"
  );

  return result;
}

uint32_t write_string_len_no_upd(const char *buf, uint32_t len)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_WRITE_STR_LEN_NO_UPD), "b" ((uint32_t) buf), "c" (len): "memory"
  );

  return result;
}

uint32_t set_scrollback(bool v)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_SET_SCROLLBACK), "b" ((uint32_t) v) : "memory"
  );

  return result;
}

uint32_t get_cursor(uint32_t *row, uint32_t *col)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_GET_CURSOR), "b" (row), "c" (col): "memory"
  );

  return result;
}
uint32_t get_epoch(uint64_t *epoch_out)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_GET_EPOCH), "b" ((uint32_t) epoch_out) : "memory"
  );

  return result;
}

uint32_t fmove(uint32_t old_parent_n, uint32_t new_parent_n, const char *old_name, const char *new_name)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_RENAME_INODE), "b" (old_parent_n), "d" (new_parent_n), "c" ((uint32_t) old_name), "S" ((uint32_t) new_name) : "memory"
  );

  return result;
}
