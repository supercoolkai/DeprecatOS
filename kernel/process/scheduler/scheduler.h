#ifndef SCHEDULER_H
#define SCHEDULER_H
#define STACK_SIZE 16384

#ifndef __ASSEMBLER__
#include <stdint.h>
#include "memory/boundaries/boundaries.h"
#endif

#define KERNEL_FRAME_SIZE 52
#define USER_FRAME_SIZE 60

#define FRAME_EIP_INDEX 10
#define FRAME_CS_INDEX 11
#define FRAME_EFLAGS_INDEX 12
#define FRAME_USER_ESP_INDEX 13
#define FRAME_SS_INDEX 14

#define INTERRUPT_AND_RESERVED_EFLAGS 0x202

#ifndef __ASSEMBLER__
uint32_t schedule(uint32_t esp);
void scheduler_init(void);
void create_process(void (*entry)(void));
void create_user_process(uint32_t entry, uint32_t user_stack_top, struct region_row *rows);
uint32_t kill_current(void);
struct region_row *get_current_rows(void);

#endif

#endif
