#include "min/min.h"
#include <stdint.h>

uint64_t min(uint64_t a, uint64_t b)
{
  if (a < b) return a;
  return b;
}
