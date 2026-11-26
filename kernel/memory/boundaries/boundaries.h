#ifndef BOUNDARIES_H
#define BOUNDARIES_H

#include <stdbool.h>
#include <stdint.h>

#define KERNEL_END 0x40000000
#define USER_END 0x100000000

#define MAX_ROWS 8

extern uint32_t kernel_ceiling;

struct region_row
{
  uint64_t addr_lo; // inclusive
  uint64_t addr_hi; // exclusive
  
  uint32_t flags;
};

bool check_in_bounds(struct region_row *rows, uint64_t base, uint64_t length);

#endif
