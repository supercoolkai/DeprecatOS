#ifndef TIMER_CONTROLLER_H
#define TIMER_CONTROLLER_H
#include <stdint.h>

void timer_init(void);
void idt_set_gate(int n, void (*handler) (void));
void idt_set_gate_type(int n, void (*handler)(void), uint8_t type);
uint32_t timer_get_tick(void);

#endif
