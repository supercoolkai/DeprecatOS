#ifndef FRAMEALLOCATOR_H
#define FRAMEALLOCATOR_H

#include "memory/mmap/memoryMap.h"
#include "memory/structs/physAddr.h"

#define FRAME_BITMAP_LENGTH 1048576
#define FRAME_BITMAP_LENGTH_NO_PAE 32768
#define FRAME_SIZE_BYTES 4096

void frame_alloc_init(const MBIInfo *info);
phys_addr_t alloc_frame(void);
void free_frame(phys_addr_t addr);

#endif
