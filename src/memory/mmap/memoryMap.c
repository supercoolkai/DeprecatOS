#include "memory/mmap/memoryMap.h"
#include "util/kprintf/kprintf.h"
#include "drivers/fb/fbController.h"
#include "drivers/serial/serialController.h"
#include "util/hex/hexPrinter.h"
#include <stdint.h>



static void print_entry(uint32_t base, uint32_t len, uint32_t type)
{
  kprintf(KPRINTF_GREEN "base: " KPRINTF_RESET);
  print_hex(base, GREEN);

  kprintf(KPRINTF_GREEN "\nlen: " KPRINTF_RESET);
  print_hex(len, GREEN);

  kprintf(KPRINTF_GREEN "\ntype: " KPRINTF_RESET);
  print_hex(type, GREEN);

  kprintf(KPRINTF_GREEN "\n\n" KPRINTF_RESET);
}



void mmap_print(const MBIInfo *info)
{
  mmap_walk(info, print_entry);
}


void mmap_walk(const MBIInfo *info, void (*fn)(uint32_t base, uint32_t len, uint32_t type))
{
  if ((info->flags & (1 << 6)) == 0){
    kprintf(KPRINTF_YELLOW "Bad flags" KPRINTF_RESET);
    return;
  }
  
  MemoryMapEntry *entry;

  for (uint32_t i = info->mmap_addr; i < info->mmap_addr + info-> mmap_length; i += entry->size+4){
    entry = (MemoryMapEntry *)i;
    
    if (entry->baseHi != 0)
      continue;

    uint32_t len = entry->lenLo;

    if (entry->lenHi != 0 || entry->baseLo + len < entry->baseLo) {
      len = 0xFFFFFFFF - entry->baseLo + 1;
    }

    fn(entry->baseLo, len, entry->type);
  }
}

