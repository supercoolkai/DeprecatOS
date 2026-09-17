#ifndef KERNELHEAP_H
#define KERNELHEAP_H

#include <stdint.h>

#define HEAP_START 0x400000
#define HEAP_END_MIN 0x401000

void heap_init(void);
void* kmalloc(uint32_t size);
void kfree(void *ptr);

#endif
