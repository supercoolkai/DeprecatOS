#ifndef LONGOPS_H
#define LONGOPS_H
#include <stdint.h>

uint64_t __udivmoddi4(uint64_t n, uint64_t d, uint64_t *rem);
uint64_t __udivdi3(uint64_t n, uint64_t d);
uint64_t __umoddi3(uint64_t n, uint64_t d);

#endif
