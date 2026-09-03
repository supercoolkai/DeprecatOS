#include <stdint.h>
#include "portio.h"
#include "drivers/serial/serialController.h"

#define UART_CLOCK_HZ 115200u
#define BAUD_RATE 115200u

#define COM1 0x3F8

void serial_init(void)
{
  outb(COM1 + 1, 0x00);

  outb(COM1 + 3, 0x80);
  uint16_t divisor = UART_CLOCK_HZ / BAUD_RATE;
  outb(COM1 + 0, (uint8_t)(divisor & 0xFF));
  outb(COM1 + 1, (uint8_t)(divisor >> 8));

  outb(COM1 + 3, 0x03);

  outb(COM1 + 2, 0xC7);

  outb(COM1 + 4, 0x03);
}

uint8_t serial_receive_char(void)
// 0 = no byte avail
{
  if (!(inb(COM1 + 5) & 0x01))
    return 0;

  return inb(COM1 + 0);
}

void serial_receive(uint8_t *buffer, uint32_t size)
{
  for(uint32_t i =0; i < size; i++)
  {
    buffer[i] = serial_receive_char();
  }
}

void serial_write_char(char c)
{
  while (!(inb(COM1 + 5) & 0x20))
    ;

  outb(COM1 + 0, (uint8_t)c);
}

void serial_write_string(const char *str)
{
  while(*str){
    serial_write_char(*str);
    str++;
  }
}
