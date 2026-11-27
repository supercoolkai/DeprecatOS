#include "memory/paging/paging.h"
#include <stdint.h>
#include "memory/frameAlloc/frameAllocator.h"
#include "memory/mmap/memoryMap.h"
#include "kprintf/kprintf.h"
#include "hex/hexPrinter.h"
#include "colors/colors.h"
#include "memory/heap/kernelHeap.h"
#include "cpu/cpu.h"

static uint32_t *kernel_dir;
static uint64_t kernel_pdpt[PDPT_SIZE / sizeof(uint64_t)]
__attribute__((aligned(32)));
static uint32_t ram_top;

static void track_ram_top(uint64_t base, uint64_t len, uint32_t type)
{
  if (type != 1) return;
  uint64_t top = base + len;
  if (top < base) top = 0xFFFFFFFF;
  if (top > HEAP_START) top = HEAP_START;
  if (top > ram_top || ram_top > HEAP_START){
    ram_top = top;
  }
}

static void map_page(uint32_t *dir, uint32_t virt, phys_addr_t phys, uint32_t flags)
{
  uint32_t di = virt >> 22;

  if(!(dir[di] & PAGE_PRESENT)) {
    phys_addr_t table_frame = alloc_frame();

    if (table_frame.addr >= HEAP_START || table_frame.addr == 0) {
      if (table_frame.addr != 0) free_frame(table_frame);
      kprintf(KPRINTF_RED "map_page: FAILED TO MAP PAGE, MEMORY OUT OF RANGE, TABLE ADDR  = " KPRINTF_RESET);
      print_hex((uint32_t)(table_frame.addr >> 32), RED);
      print_hex((uint32_t)table_frame.addr, RED);
      kprintf(KPRINTF_RED "\n" KPRINTF_RESET);
      return;
    }

    uint32_t table_addr = (uint32_t)table_frame.addr;
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

  table[(virt >> 12) & 0x3FF] = (uint32_t)phys.addr | flags;
}

static void map_pae_page(uint64_t *pdpt, uint32_t virt, phys_addr_t phys, uint32_t flags)
{
  uint8_t pdpt_entry = virt >> PDPT_ENTRY_OFFSET;
  if (!(pdpt[pdpt_entry] & PAGE_PRESENT)) {
    phys_addr_t table_frame = alloc_frame();

    if (table_frame.addr >= HEAP_START || table_frame.addr == 0) {
      if (table_frame.addr != 0) free_frame(table_frame);
      kprintf(KPRINTF_RED "map_pae_page: FAILED TO MAP PDPT ENTRY, MEMORY OUT OF RANGE, TABLE ADDR  = " KPRINTF_RESET);
      print_hex((uint32_t)(table_frame.addr >> 32), RED);
      print_hex((uint32_t)table_frame.addr, RED);
      kprintf(KPRINTF_RED "\n" KPRINTF_RESET);
      return;
    }

    uint32_t table_addr = (uint32_t) table_frame.addr;
    uint64_t *table = (uint64_t *)table_addr;

    for(int i = 0; i < 512; i++){
      table[i] = 0;
    }

    pdpt[pdpt_entry] = table_addr | PAGE_PRESENT;
  }
  
  uint64_t *dir = (uint64_t *)((uint32_t)(pdpt[pdpt_entry] & ~0xFFFull));

  uint64_t di = (virt >> 21) & 0x1FF;

  if(!(dir[di] & PAGE_PRESENT)) {
    phys_addr_t table_frame = alloc_frame();

    if (table_frame.addr >= HEAP_START || table_frame.addr == 0) {
      if (table_frame.addr != 0) free_frame(table_frame);
      kprintf(KPRINTF_RED "map_pae_page: FAILED TO MAP PAGE, MEMORY OUT OF RANGE, TABLE ADDR  = " KPRINTF_RESET);
      print_hex((uint32_t)(table_frame.addr >> 32), RED);
      print_hex((uint32_t)table_frame.addr, RED);
      kprintf(KPRINTF_RED "\n" KPRINTF_RESET);
      return;
    }

    uint32_t table_addr = (uint32_t) table_frame.addr;
    uint64_t *table = (uint64_t *)table_addr;

    for(int i = 0; i < 512; i++){
      table[i] = 0;
    }

    dir[di] = table_addr | PAGE_PRESENT | PAGE_RW;
  }

  if ((dir[di] & ~0xFFFull) >= HEAP_START || (dir[di] & ~0xFFFull) == 0) {
    kprintf(KPRINTF_RED "map_pae_page: FAILED TO MAP PAGE, MEMORY OUT OF RANGE, TABLE ADDR: " KPRINTF_RESET); 
    print_hex((uint32_t)(dir[di] >> 32), RED);
    print_hex((uint32_t)dir[di] & ~0xFFFu, RED);
    kprintf(KPRINTF_RED "\n" KPRINTF_RESET);
    return;
  }
  
  if (flags & PAGE_USER){
    dir[di] |= PAGE_USER;
  }
  
  uint64_t *table = (uint64_t *)(((uint32_t) dir[di]) & ~0xFFFu);

  table[(virt >> 12) & 0x1FF] = phys.addr | flags;
}

void map_kernel_page(uint32_t virt, phys_addr_t phys, uint32_t flags)
{
  if (pae_on){
    map_pae_page(kernel_pdpt, virt, phys, flags);
    return;
  }

  map_page(kernel_dir, virt, phys, flags);
}

static phys_addr_t unmap_page(uint32_t *dir, uint32_t virt)
{
  uint32_t di = virt >> 22;

  if(!(dir[di] & PAGE_PRESENT))
    return (phys_addr_t){0};

  uint32_t *table = (uint32_t *)(dir[di] & ~0xFFFu);
  uint32_t ti = (virt >> 12) & 0x3FF;

  if(!(table[ti] & PAGE_PRESENT))
    return (phys_addr_t){0};

  uint32_t phys = table[ti] & ~0xFFFu;
  table[ti] = 0;

  __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");

  return (phys_addr_t){phys};
}

static phys_addr_t unmap_pae_page(uint64_t *pdpt, uint32_t virt)
{
  uint8_t pdpt_entry = virt >> PDPT_ENTRY_OFFSET;
  if (!(pdpt[pdpt_entry] & PAGE_PRESENT))
    return (phys_addr_t){0};

  uint64_t *dir = (uint64_t *)((uint32_t)(pdpt[pdpt_entry] & ~0xFFFull));
  uint64_t di = (virt >> 21) & 0x1FF;

  if(!(dir[di] & PAGE_PRESENT))
    return (phys_addr_t){0};

  uint64_t *table = (uint64_t *)((uint32_t)(dir[di] & ~0xFFFull));
  uint64_t ti = (virt >> 12) & 0x1FF;

  if(!(table[ti] & PAGE_PRESENT))
    return (phys_addr_t){0};

  phys_addr_t phys = {table[ti] & ~0xFFFull};
  table[ti] = 0;

  __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");

  return phys;
}

phys_addr_t unmap_kernel_page(uint32_t virt)
{
  if (pae_on)
    return unmap_pae_page(kernel_pdpt, virt);
  return unmap_page(kernel_dir, virt);
}

void paging_init(MBIInfo *info)
{
  if (pae_on)
  {
    mmap_walk(info, track_ram_top);

    for (int i = 0; i < (PDPT_SIZE / sizeof(uint64_t)); i++) {
      kernel_pdpt[i] = 0;
    }

    for (uint32_t a = 0x1000; a < ram_top; a+=0x1000) {
      map_pae_page(kernel_pdpt,a, (phys_addr_t){a}, PAGE_PRESENT | PAGE_RW);
    }

    paging_enable((uint32_t) kernel_pdpt);

    return;
  }

  kernel_dir = (uint32_t *)(uint32_t)alloc_frame().addr;

  mmap_walk(info, track_ram_top);

  for(int i = 0; i < 1024; i++){
    kernel_dir[i] = 0;
  }

  for(uint32_t a = 0x1000; a < ram_top; a += 0x1000) {
    map_page(kernel_dir, a, (phys_addr_t){a}, PAGE_PRESENT | PAGE_RW);
  }

  paging_enable((uint32_t) kernel_dir);
}


