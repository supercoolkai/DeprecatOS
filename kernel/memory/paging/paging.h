#ifndef PAGING_H
#define PAGING_H
#define PAGE_PRESENT 1
#define PAGE_RW (1 << 1)
#define PAGE_USER (1 << 2)
#define PAGE_GROWABLE (1 << 9)
#define PAGE_NX (1 << 10)
#define ENTRY_NX (1ull << 63) // NOTE: to be ORed as a flag alongside PAGE_NX
#define PAE_ENTRY_ADDR_MASK 0x000FFFFFFFFFF000ull
#define PAGE_SIZE 4096
#define PDPT_SIZE 32
#define PDPT_ENTRY_OFFSET 30 // for virt

#define PDPT_SELF_POINT_BASE 508
#define PDPT_SELF_POINT_AMT 4

#define KERNEL_DIR_SELF_POINT 255

#define PAE_WINDOW_BASE (PDPT_SELF_POINT_BASE << 21)
#define WINDOW_BASE (KERNEL_DIR_SELF_POINT << 22)

#include <stdint.h>
#include "memory/mmap/memoryMap.h"
#include "memory/structs/physAddr.h"

void paging_init(MBIInfo *info);
void paging_enable(uint32_t dir_phys);
void map_kernel_page(uint32_t virt, phys_addr_t phys, uint32_t flags);
phys_addr_t unmap_kernel_page(uint32_t virt);
uint32_t read_cr2(void);

#endif
