#include <stdint.h>
#include "portio.h"
#include "interrupts/idtController.h"

struct idt_entry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t  zero;
  uint8_t  type_attr;
  uint16_t offset_high;
} __attribute__((packed));

struct idt_ptr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtp;

extern void irq1_stub(void);
extern void default_isr_stub(void);

void idt_set_gate_type(int n, void (*handler)(void), uint8_t type)
{
  uint32_t addr = (uint32_t)handler;
  idt[n].offset_low  = addr & 0xFFFF;
  idt[n].selector    = 0x08;
  idt[n].zero        = 0;
  idt[n].type_attr   = type;
  idt[n].offset_high = addr >> 16;
}

void idt_set_gate(int n, void (*handler)(void))
{
  idt_set_gate_type(n, handler, 0x8E);
}

void idt_init(void)
{
  for (int i = 0; i < 256; i++)
    idt_set_gate(i, default_isr_stub);

  idt_set_gate(0x21, irq1_stub);

  idtp.limit = sizeof(idt) - 1;
  idtp.base  = (uint32_t)idt;
  __asm__ volatile ("lidt %0" : : "m"(idtp));

  outb(PIC1_CMD,  0x11);
  outb(PIC2_CMD,  0x11);
  outb(PIC1_DATA, 0x20);
  outb(PIC2_DATA, 0x28);
  outb(PIC1_DATA, 0x04);
  outb(PIC2_DATA, 0x02);
  outb(PIC1_DATA, 0x01);
  outb(PIC2_DATA, 0x01);

  outb(PIC1_DATA, 0xFC);
  outb(PIC2_DATA, 0xFF);
}
