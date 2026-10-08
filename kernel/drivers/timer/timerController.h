#ifndef TIMER_CONTROLLER_H
#define TIMER_CONTROLLER_H
#include <stdint.h>

void timer_init(void);
uint64_t timer_get_tick(void);
uint64_t timer_get_epoch(void);
uint32_t timer_get_epoch_sec(void);

#endif
