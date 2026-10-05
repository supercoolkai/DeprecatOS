#ifndef SERIAL_CONTROLLER_H
#define SERIAL_CONTROLLER_H

#include <stdint.h>

void serial_init(void);
uint8_t serial_receive_char(void);
void serial_receive(uint8_t *buffer, uint32_t size);
void serial_write_char(char c);
void serial_write_string(const char *str);

#endif
