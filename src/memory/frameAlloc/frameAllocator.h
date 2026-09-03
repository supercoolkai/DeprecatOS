#ifndef FRAMEALLOCATOR_H
#define FRAMEALLOCATOR_H

#include "memory/mmap/memoryMap.h"


void frame_alloc_init(const MBIInfo *info);
uint32_t alloc_frame(void);
void free_frame(uint32_t addr);

#endif
