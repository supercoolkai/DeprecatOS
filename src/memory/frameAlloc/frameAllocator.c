#include <stdint.h>
#include "memory/frameAlloc/frameAllocator.h"
#include "memory/mmap/memoryMap.h"

static uint32_t frame_bitmap[32768];
extern char _ebss[];

static void mark_free_region(uint32_t base, uint32_t len, uint32_t type)
{
  if (type != 1) return;

  uint32_t first = ((base + 0xFFF) & ~0xFFFu) >> 12;
  uint32_t last = ((base + len) & ~0xFFFu) >> 12;

  for(uint32_t i = first; i < last; i++){
    frame_bitmap[i / 32] &= ~(1u << (i % 32));
  }
}

void frame_alloc_init(const MBIInfo *info)
{
  for (uint32_t i = 0; i < 32768; i++){
    frame_bitmap[i] = 0xFFFFFFFF;
  }
  
  mmap_walk(info, mark_free_region);

  uint32_t last = ((((uint32_t)_ebss) + 0xFFF) & ~0xFFFu) >> 12;
  for(uint32_t i = 0; i < last; i++) {
    frame_bitmap[i / 32] |= 1u << (i % 32);
  }
}

uint32_t alloc_frame(void)
{
  for (uint32_t i = 0; i < 32768; i++){
    if (frame_bitmap[i] != 0xFFFFFFFF){
      for (uint32_t b = 0; b < 32; b++) {
        if (((frame_bitmap[i] >> b) & 1) == 0) {
          frame_bitmap[i] |= 1u << b;
          return (i * 32 + b) * 4096;
        }
      }
    }
  }
  return 0;
}

void free_frame(uint32_t addr)
{
  uint32_t f = addr >> 12;

  frame_bitmap[f / 32] &= ~(1u << (f % 32));
}

