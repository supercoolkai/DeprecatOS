#include "memory/paging/paging.h"
#include <stdint.h>
#include "memory/frameAlloc/frameAllocator.h"
#include "memory/mmap/memoryMap.h"
#include "util/kprintf/kprintf.h"
#include "util/hex/hexPrinter.h"
#include "drivers/fb/fbController.h"
#include "memory/heap/kernelHeap.h"

static uint32_t *kernel_dir;
static uint32_t ram_top;

static void track_ram_top(uint32_t base, uint32_t len, uint32_t type)
{
  if (type != 1) return;
  uint32_t top = base + len;
  if (top < base) top = 0xFFFFFFFF;
  if (top > HEAP_START) top = HEAP_START;
  if (top > ram_top || ram_top > HEAP_START){
    ram_top = top;
  }
}

static void map_page(uint32_t *dir, uint32_t virt, uint32_t phys, uint32_t flags)
{
  uint32_t di = virt >> 22;

  if(!(dir[di] & PAGE_PRESENT)) {
    uint32_t table_addr = alloc_frame();

    if (table_addr >= HEAP_START || table_addr == 0) {
      if (table_addr != 0) free_frame(table_addr);
      kprintf(KPRINTF_RED "map_page: FAILED TO MAP PAGE, MEMORY OUT OF RANGE, TABLE ADDR  = " KPRINTF_RESET);
      print_hex(table_addr, RED);
      kprintf(KPRINTF_RED "\n" KPRINTF_RESET);
      return;
    }
    
    uint32_t *table = (uint32_t *)table_addr;

    for(int i = 0; i < 1024; i++){
      table[i] = 0;
    }

    dir[di] = table_addr | PAGE_PRESENT | PAGE_RW;
  }

  if ((dir[di] & ~0xFFFu) >= HEAP_START || (dir[di] & ~0xFFFu) == 0) {
    kprintf(KPRINTF_RED "map_page: FAILED TO MAP PAGE, MEMORY OUT OF RANGE, TABLE ADDR: " KPRINTF_RESET); 
    print_hex((dir[di] & ~0xFFFu), RED);
    kprintf(KPRINTF_RED "\n" KPRINTF_RESET);
    return;
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

void paging_init(MBIInfo *info)
{
  kernel_dir = (uint32_t *) alloc_frame();

  mmap_walk(info, track_ram_top);

  for(int i = 0; i < 1024; i++){
    kernel_dir[i] = 0;
  }

  for(uint32_t a = 0x1000; a < ram_top; a += 0x1000) {
    map_page(kernel_dir, a, a, PAGE_PRESENT | PAGE_RW);
  }

  paging_enable((uint32_t) kernel_dir);
}


