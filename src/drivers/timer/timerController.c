#include <stdint.h>
#include "portio.h"
#include "drivers/timer/timerController.h"
#include "idt/idtController.h"
#include "process/scheduler/scheduler.h"

#define TARGET_HZ 1000u
#define PIT_HZ 1193182u

#define PIT_CH0  0x40
#define PIT_CMD  0x43

static volatile uint32_t tick = 0;

extern void irq0_stub(void);

void timer_init(void)
{
  idt_set_gate(0x20, irq0_stub);

  uint16_t divisor = (uint16_t)(PIT_HZ / TARGET_HZ);
  outb(PIT_CMD, 0x36);
  outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
  outb(PIT_CH0, (uint8_t)(divisor >> 8));

  __asm__ volatile ("sti");
}

uint32_t timer_irq_handler(uint32_t esp)
{
  tick++;

  outb(PIC1_CMD, 0x20);

  return schedule(esp);
}

uint32_t timer_get_tick(void)
{
  return tick;
}
