#include "memory/paging/paging.h"
#include <stdint.h>
#include "memory/frameAlloc/frameAllocator.h"

static uint32_t *kernel_dir;

static void map_page(uint32_t *dir, uint32_t virt, uint32_t phys, uint32_t flags)
{
  uint32_t di = virt >> 22;

  if(!(dir[di] & PAGE_PRESENT)) {
    uint32_t table_addr = alloc_frame();
    
    uint32_t *table = (uint32_t *)table_addr;

    for(int i = 0; i < 1024; i++){
      table[i] = 0;
    }

    dir[di] = table_addr | PAGE_PRESENT | PAGE_RW;
  }
  
  if (flags & PAGE_USER){
    dir[di] |= PAGE_USER;
  }
  
  uint32_t *table = (uint32_t *)(dir[di] & ~0xFFFu);

  table[(virt >> 12) & 0x3FF] = phys | flags;
}

void map_kernel_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
  map_page(kernel_dir, virt, phys, flags);
}

uint32_t unmap_kernel_page(uint32_t virt)
{
  uint32_t di = virt >> 22;

  if(!(kernel_dir[di] & PAGE_PRESENT))
    return 0;

  uint32_t *table = (uint32_t *)(kernel_dir[di] & ~0xFFFu);
  uint32_t ti = (virt >> 12) & 0x3FF;

  if(!(table[ti] & PAGE_PRESENT))
    return 0;

  uint32_t phys = table[ti] & ~0xFFFu;
  table[ti] = 0;

  __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");

  return phys;
}

void paging_init(void)
{
  kernel_dir = (uint32_t *) alloc_frame();

  for(int i = 0; i < 1024; i++){
    kernel_dir[i] = 0;
  }

  for(uint32_t a = 0x1000; a < 0x400000; a += 0x1000) {
    map_page(kernel_dir, a, a, PAGE_PRESENT | PAGE_RW);
  }

  paging_enable((uint32_t) kernel_dir);
}


