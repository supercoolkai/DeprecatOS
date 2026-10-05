#include "gdt/tssController.h"
#include "gdt/segments.h"
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
  info.ss0 = KERNEL_DATA_SEGMENT_SEL;
  info.IOBitmapOffset = sizeof(tssInfo) << 16;
  

  // check the table in docs/gdt.md 
  // if you don't know what these mean
  uint32_t base = (uint32_t)&info;
  gdt[TSS_GDT_ENTRY_IND].limit  = sizeof(tssInfo) - 1;
  gdt[TSS_GDT_ENTRY_IND].baseBits = base & 0xFFFF;
  gdt[TSS_GDT_ENTRY_IND].baseBitsContd = (base>>16) & 0xFF;
  gdt[TSS_GDT_ENTRY_IND].accessByte = 0x89;
  gdt[TSS_GDT_ENTRY_IND].flags = 0;
  gdt[TSS_GDT_ENTRY_IND].baseBitsTop = base >> 24;

  uint16_t sel = TSS_GDT_ENTRY_IND << 3;
  __asm__ volatile ("ltr %0" : : "r"(sel));
}

void tss_set_esp(uint32_t n)
{
  info.esp0 = n;
}


