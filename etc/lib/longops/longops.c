#include "longops/longops.h"
#include <stdint.h>

uint64_t __udivmoddi4(uint64_t n, uint64_t d, uint64_t *rem)
{
  if (d == 0) // force division error 
    __asm__ volatile ("xor %%edx, %%edx; divl %0" :: "r"((uint32_t)0) : "eax", "edx", "cc");

  uint64_t q = 0;
  uint64_t r = 0;

  for (int i = 0; i < 64; i++) {
    r = (r << 1) | (n >> 63);

    n <<= 1;
    q <<= 1;

    if (r >= d){
      r -= d;
      q |= 1;
    }
  }

  if (rem)
    *rem = r;
  return q;
}

uint64_t __udivdi3(uint64_t n, uint64_t d)
{
  return __udivmoddi4(n, d, 0);
}

uint64_t __umoddi3(uint64_t n, uint64_t d)
{
  uint64_t r;
  __udivmoddi4(n, d, &r);
  return r;
}
