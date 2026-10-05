#include "mem.h"
#include <stdint.h>
#include <stddef.h>

static void *copy_fwd(void *dst, const void *src, size_t n) {
  uint8_t *d = (uint8_t *) dst;
  const uint8_t *s = (const uint8_t *) src;
  uint32_t dwords = n>>2;
  uint32_t bytes = n&3;

  __asm__ volatile ("cld\n\trep movsl" : "+D"(d), "+S"(s), "+c"(dwords) :: "memory");
  __asm__ volatile ("cld\n\trep movsb" : "+D"(d), "+S"(s), "+c"(bytes) :: "memory");

  return dst;
}

static void *copy_bwd(void *dst, const void *src, size_t n){
  if (n == 0 || dst == src) return dst;

  uint8_t *d = (uint8_t *) dst + n - 1;
  const uint8_t *s = (const uint8_t *) src + n - 1;
  
  __asm__ volatile ("std\n\trep movsb\n\tcld" : "+D"(d), "+S"(s), "+c"(n) :: "memory");

  return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
  return copy_fwd(dst, src, n);
}

void *memmove(void *dst, const void *src, size_t n) {
  return ((uintptr_t) dst < (uintptr_t) src) ? copy_fwd(dst, src, n) : copy_bwd(dst, src, n);
}

void *memset(void *dst, int c, size_t n)
{
  unsigned char *d = (unsigned char *) dst;
  uint32_t word = (uint8_t)c * 0x01010101u;
  uint32_t dwords = n >> 2;
  uint32_t bytes = n & 3;

  __asm__ volatile ("cld\n\trep stosl" : "+D"(d), "+c" (dwords) : "a"(word) : "memory");
  __asm__ volatile ("cld\n\trep stosb" : "+D"(d), "+c"(bytes) : "a"(word) : "memory");

  return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
  const unsigned char *lhs = (const unsigned char *) a;
  const unsigned char *rhs = (const unsigned char *) b;

  for (size_t b = 0; b < n; b++) {
    if (lhs[b] != rhs[b])
      return lhs[b] - rhs[b];
  }

  return 0;
}
