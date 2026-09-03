#ifndef PROCESSQUEUE_H
#define PROCESSQUEUE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint32_t esp;
  uint32_t stack_addr;

  uint32_t malloc_addr;
} Process;

void pqueue_init(void);
Process *pqueue_pop(void);
bool pqueue_push(Process *p);
Process *pqueue_peek(void);
bool pqueue_is_empty(void);
unsigned char pqueue_size(void);

#endif
