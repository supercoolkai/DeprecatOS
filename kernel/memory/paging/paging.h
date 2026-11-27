#ifndef PAGING_H
#define PAGING_H
#define PAGE_PRESENT 1
#define PAGE_RW (1 << 1)
#define PAGE_USER (1 << 2)
#define PAGE_GROWABLE (1 << 9)
#define PAGE_SIZE 4096

#include <stdint.h>
#include "memory/mmap/memoryMap.h"
#include "memory/structs/physAddr.h"

void paging_init(MBIInfo *info);
void paging_enable(uint32_t dir_phys);
void map_kernel_page(uint32_t virt, phys_addr_t phys, uint32_t flags);
phys_addr_t unmap_kernel_page(uint32_t virt);
uint32_t read_cr2(void);

#endif
