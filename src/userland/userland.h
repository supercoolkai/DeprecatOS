#ifndef USERLAND_H
#define USERLAND_H

#define MISC_PAGE_CNT 3
#define STACK_ADDR (KERNEL_CEILING + 0x1000 * (MISC_PAGE_CNT + 1))
#define CODE_ADDR KERNEL_CEILING
#define USER_SPACE_END (KERNEL_CEILING + (0x1000 * (MISC_PAGE_CNT + 2)))

void userland_init(void);
void userland_teardown(void);

#endif
