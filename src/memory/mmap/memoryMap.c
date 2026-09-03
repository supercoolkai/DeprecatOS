#include "memory/mmap/memoryMap.h"
#include "drivers/fb/fbController.h"
#include "drivers/serial/serialController.h"
#include "util/hex/hexPrinter.h"
#include <stdint.h>



static void print_entry(uint32_t base, uint32_t len, uint32_t type)
{
  serial_write_string("base: ");
  fb_draw_string("base: ", GREEN);
  print_hex(base, GREEN);

  serial_write_string("\nlen: ");
  fb_draw_string("\nlen: ", GREEN);
  print_hex(len, GREEN);

  serial_write_string("\ntype: ");
  fb_draw_string("\ntype: ", GREEN);
  print_hex(type, GREEN);

  serial_write_string("\n\n");
  fb_draw_string("\n\n", GREEN);
}



void mmap_print(const MBIInfo *info)
{
  mmap_walk(info, print_entry);
}


void mmap_walk(const MBIInfo *info, void (*fn)(uint32_t base, uint32_t len, uint32_t type))
{
  if ((info->flags & (1 << 6)) == 0){
    serial_write_string("Bad flags");
    fb_draw_string("Bad flags", YELLOW);
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

