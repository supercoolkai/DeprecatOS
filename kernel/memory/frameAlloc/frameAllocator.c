#include <stdint.h>
#include "memory/frameAlloc/frameAllocator.h"
#include "memory/mmap/memoryMap.h"
#include "cpu/cpu.h"
#include "min/min.h"

static uint32_t frame_bitmap[FRAME_BITMAP_LENGTH];
extern char _ebss[];

static void mark_free_region(uint64_t base, uint64_t len, uint32_t type)
{
  if (type != 1) return;

  uint64_t first = ((base + 0xFFF) & ~0xFFFull) >> 12;
  uint64_t last = min(((base + len) & ~0xFFFull) >> 12, FRAME_BITMAP_LENGTH * 32);
  
  if (!pae_on)
    last = min(last, FRAME_BITMAP_LENGTH_NO_PAE * 32);

  for(uint64_t i = first; i < last; i++){
    frame_bitmap[i / 32] &= ~(1u << (i % 32));
  }
}

void frame_alloc_init(const MBIInfo *info)
{
  for (uint32_t i = 0; i < FRAME_BITMAP_LENGTH; i++){
    // means all frames used, NOT an error
    // like in syscalls.
    frame_bitmap[i] = 0xFFFFFFFF;
  }
  
  mmap_walk(info, mark_free_region);

  uint32_t last = ((((uint32_t)_ebss) + 0xFFF) & ~0xFFFu) >> 12;
  for(uint32_t i = 0; i < last; i++) {
    frame_bitmap[i / 32] |= 1u << (i % 32);
  }
}

phys_addr_t alloc_frame(void)
{
  for (uint64_t i = 0; i < FRAME_BITMAP_LENGTH; i++){
    if (frame_bitmap[i] != 0xFFFFFFFF){
      for (uint64_t b = 0; b < 32; b++) {
        if (((frame_bitmap[i] >> b) & 1) == 0) {
          frame_bitmap[i] |= 1u << b;
          return (phys_addr_t){(i * 32 + b) * FRAME_SIZE_BYTES};
        }
      }
    }
  }
  return (phys_addr_t){0};
}

void free_frame(phys_addr_t addr)
{
  uint64_t f = addr.addr >> 12;

  frame_bitmap[f / 32] &= ~(1u << (f % 32));
}

