#include "memory/boundaries/boundaries.h"

uint32_t kernel_ceiling = KERNEL_END;

bool check_in_bounds(struct region_row *rows, uint64_t base, uint64_t length)
{
  for (uint32_t r = 0; r < MAX_ROWS; r++) {
    if (rows[r].addr_lo <= base && rows[r].addr_hi >= base + length) 
      return true;
  }

  return false;
}
