#ifndef IDT_CONTROLLER_H
#define IDT_CONTROLLER_H
#include <stdint.h>

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

void idt_init(void);
void idt_set_gate(int n, void (*handler) (void));
void idt_set_gate_type(int n, void (*handler)(void), uint8_t type);

#endif
