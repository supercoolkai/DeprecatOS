#ifndef USERLAND_H
#define USERLAND_H

#include "memory/boundaries/boundaries.h"

#define MISC_PAGE_CNT 255
#define STACK_ADDR (KERNEL_END + 0x1000 * (MISC_PAGE_CNT + 1))
#define CODE_ADDR KERNEL_END
#define USER_SPACE_END (KERNEL_END + (0x1000 * (MISC_PAGE_CNT + 2)))

void userland_init(void);
void userland_teardown(void);

#endif
