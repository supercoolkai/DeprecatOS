#include "process/cpu/tssController.h"
#include <stdint.h>

static tssInfo info;

struct gdt_entry{
  uint16_t limit;
  uint16_t baseBits;
  uint8_t baseBitsContd;
  uint8_t accessByte;
  uint8_t flags;
  uint8_t baseBitsTop;
} __attribute__((packed));

extern char stack_top[];
extern struct gdt_entry gdt[];

void tss_init(void)
{
  info.esp0 = (uint32_t) stack_top;
  info.ss0 = 0x10;
  info.IOBitmapOffset = 104 << 16;
  
  uint32_t base = (uint32_t)&info;
  gdt[5].limit  = 103;
  gdt[5].baseBits = base & 0xFFFF;
  gdt[5].baseBitsContd = (base>>16) & 0xFF;
  gdt[5].accessByte = 0x89;
  gdt[5].flags = 0;
  gdt[5].baseBitsTop = base >> 24;

  uint16_t sel = 0x28;
  __asm__ volatile ("ltr %0" : : "r"(sel));
}

void tss_set_esp(uint32_t n)
{
  info.esp0 = n;
}


