#include "shell/sys/syscall.h"
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
uint32_t get_ticks(void)
{
  uint32_t result;
  __asm__ volatile (
    "int $0x80"
    : "=a" (result)
    : "a" (SYS_GET_TICKS)
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
