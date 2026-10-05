#ifndef SCHEDULER_H
#define SCHEDULER_H
#define STACK_SIZE 16384

#ifndef __ASSEMBLER__
#include <stdint.h>
#endif

#define KERNEL_FRAME_SIZE 44
#define USER_FRAME_SIZE 52

#define FRAME_EIP_INDEX 8
#define FRAME_CS_INDEX 9
#define FRAME_EFLAGS_INDEX 10
#define FRAME_USER_ESP_INDEX 11
#define FRAME_SS_INDEX 12

#define INTERRUPT_AND_RESERVED_EFLAGS 0x202

#ifndef __ASSEMBLER__
uint32_t schedule(uint32_t esp);
void scheduler_init(void);
void create_process(void (*entry)(void));
void create_user_process(uint32_t entry,  uint32_t user_stack_top);
uint32_t kill_current(void);
#endif

#endif
