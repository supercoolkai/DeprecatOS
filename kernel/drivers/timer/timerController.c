#include <stdint.h>
#include "portio.h"
#include "drivers/timer/timerController.h"
#include "idt/idtController.h"
#include "process/scheduler/scheduler.h"

#define TARGET_HZ 1000u
#define PIT_HZ 1193182u

#define PIT_CH0  0x40
#define PIT_CMD  0x43

#define RTC_CENTURY_REGISTER 0x32
#define RTC_YEAR_REGISTER 0x09
#define RTC_MONTH_REGISTER 0x08
#define RTC_DAY_REGISTER 0x07
#define RTC_WEEKDAY_REGISTER 0x06
// NOTE: register 0x05 mystery reserved?? no idea why but dont question it
#define RTC_HOUR_REGISTER 0x04
#define RTC_MINUTE_REGISTER 0x02
#define RTC_SECOND_REGISTER 0x00
#define RTC_COMMAND_PORT 0x70
#define RTC_READ_PORT 0x71

#define RELEASE_YEAR 26

static volatile uint64_t tick = 0;

static uint32_t startup_epoch;

extern void irq0_stub(void);

static uint8_t cmos_read(uint8_t reg)
{
  outb(RTC_COMMAND_PORT, reg);
  return inb(RTC_READ_PORT);
}

static uint8_t bin_to_dec(uint8_t bin)
{
  return (bin & 0x0F) + (bin >> 4) * 10;
}

static uint32_t get_year_startup(void)
{
  uint8_t century = bin_to_dec(cmos_read(RTC_CENTURY_REGISTER));
  uint8_t year = bin_to_dec(cmos_read(RTC_YEAR_REGISTER));
  if (century < 20 || century > 21) {
    if (year < RELEASE_YEAR)
      century = 21;
    else
      century = 20;
  }

  return ((uint32_t) century) * 100 + ((uint32_t) year);
}

static void get_seconds_startup(void)
{
  uint32_t year = get_year_startup();
  uint32_t month = bin_to_dec(cmos_read(RTC_MONTH_REGISTER));
  uint32_t day = bin_to_dec(cmos_read(RTC_DAY_REGISTER));
  uint32_t hour = bin_to_dec(cmos_read(RTC_HOUR_REGISTER));
  uint32_t minute = bin_to_dec(cmos_read(RTC_MINUTE_REGISTER));
  uint32_t second = bin_to_dec(cmos_read(RTC_SECOND_REGISTER));
  
  // NOTE: random formula i found off the internet, dont change porfa
  year -= month <= 2;
  uint32_t era = year / 400;
  uint32_t year_of_era = year - era * 400;
  // NOTE: the (uint32_t) cast on -3 actually works cuz it wraps around and resets for march
  // so it works. only reason we casting is because were tryna store it into an unsigned value
  uint32_t day_of_year = (153 * (month + (month > 2 ? (uint32_t) -3 : 9)) + 2) / 5 + day - 1; 
  uint32_t day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;

  uint32_t day_of_epoch = era * 146097 + day_of_era - 719468;

  startup_epoch = day_of_epoch * 86400 + hour * 3600 + minute * 60 + second;
}

void timer_init(void)
{
  idt_set_gate(FIRST_IRQ_STUB_VECTOR, irq0_stub);

  uint16_t divisor = (uint16_t)(PIT_HZ / TARGET_HZ);
  outb(PIT_CMD, 0x36); // NOTE: config byte, check docs/timer.md for meaning
  outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
  outb(PIT_CH0, (uint8_t)(divisor >> 8));

  get_seconds_startup();

  __asm__ volatile ("sti");
}

uint32_t timer_irq_handler(uint32_t esp)
{
  tick++;

  outb(PIC1_CMD, PIC_EOI);

  return schedule(esp);
}

uint64_t timer_get_tick(void)
{
  return tick;
}

uint64_t timer_get_epoch(void)
{
  return ((uint64_t) startup_epoch * 1000) + tick;
}

uint32_t timer_get_epoch_sec(void)
{
  return (uint32_t)(timer_get_epoch() / 1000);
}
