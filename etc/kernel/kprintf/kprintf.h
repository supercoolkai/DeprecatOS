#ifndef KPRINTF_H
#define KPRINTF_H

#define ESC_PREFIX "\x1b"
#define ESC_SUFFIX_LEN 3

#define KPRINTF_BLACK ESC_PREFIX "30"
#define KPRINTF_RED ESC_PREFIX "31"
#define KPRINTF_GREEN ESC_PREFIX "32"
#define KPRINTF_YELLOW ESC_PREFIX "33"
#define KPRINTF_BLUE ESC_PREFIX "34"
#define KPRINTF_MAGENTA ESC_PREFIX "35"
#define KPRINTF_CYAN ESC_PREFIX "36"
#define KPRINTF_WHITE ESC_PREFIX "37"
#define KPRINTF_RESET ESC_PREFIX "39"

void kprintf(const char *str);
void kprintf_init(void);

#endif
