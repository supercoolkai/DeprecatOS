#include <stdint.h>
#include "portio.h"
#include "idt/idtController.h"

#define GDT_INDEX_1 0x08
#define IDT_ENTRY_AMT 256
#define INTERRUPT_GATE_TYPE 0x8E
#define FIRST_IRQ_STUB_VECTOR 0x20

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

static struct idt_entry idt[IDT_ENTRY_AMT];
static struct idt_ptr idtp;

extern void irq1_stub(void);
extern void default_isr_stub(void);

void idt_set_gate_type(int n, void (*handler)(void), uint8_t type)
{
  uint32_t addr = (uint32_t)handler;
  idt[n].offset_low  = addr & 0xFFFF;
  idt[n].selector    = GDT_INDEX_1;
  idt[n].zero        = 0;
  idt[n].type_attr   = type;
  idt[n].offset_high = addr >> 16;
}

void idt_set_gate(int n, void (*handler)(void))
{
  idt_set_gate_type(n, handler, INTERRUPT_GATE_TYPE);
}

void idt_init(void)
{
  for (int i = 0; i < IDT_ENTRY_AMT; i++)
    idt_set_gate(i, default_isr_stub);

  idt_set_gate(FIRST_IRQ_STUB_VECTOR + 1, irq1_stub);

  idtp.limit = sizeof(idt) - 1;
  idtp.base  = (uint32_t)idt;
  __asm__ volatile ("lidt %0" : : "m"(idtp));
  
  // ooh la la magical PIC handshake
  // dont touch this 
  // in hindsight when i was adding documentation 
  // i realized that 
  // i have no idea how this works
  // but it does so im keeping it like this
  // so dont touch this pls kthx

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
