#include "max/max.h"
#include <stdint.h>

uint32_t max(uint32_t a, uint32_t b)
{
  if (a > b) return a;
  return b;
}
