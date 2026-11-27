#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

#define PREFERRED_CPUID_LEAF 1
#define PAE_BIT (1 << 6) // NOTE: this is returned in edx for leaf 1 of cpuid

extern bool pae_on;

void cpu_init(void);

#endif
