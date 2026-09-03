#ifndef PAGING_H
#define PAGING_H
#define PAGE_PRESENT 0x1
#define PAGE_RW 0x2
#define PAGE_USER 0x4
#define KERNEL_CEILING 0x40000000

#include <stdint.h>

void paging_init(void);
void paging_enable(uint32_t dir_phys);
void map_kernel_page(uint32_t virt, uint32_t phys, uint32_t flags);
uint32_t unmap_kernel_page(uint32_t virt);
uint32_t read_cr2(void);

#endif
