#ifndef SYSCALLCONTROLLER_H
#define SYSCALLCONTROLLER_H

#define SYSCALL_VECTOR_NUMBER 0x80
#define SENTINEL 0xDDDDDDDD

#include <stdint.h>

void syscall_init(void);
uint32_t syscall_handler(uint32_t esp);

#endif
