#ifndef SCHEDULER_H
#define SCHEDULER_H
#define STACK_SIZE 1024

#include <stdint.h>

uint32_t schedule(uint32_t esp);
void scheduler_init(void);
void create_process(void (*entry)(void));
void create_user_process(uint32_t entry,  uint32_t user_stack_top);
uint32_t kill_current(void);

#endif
