#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

#define PREFERRED_CPUID_LEAF 1
#define HIGH_CPUID_LEAF_TEST 0x80000000
#define NX_CPUID_LEAF 0x80000001
#define PAE_BIT (1 << 6) // NOTE: this is returned in edx for leaf 1 of cpuid
#define NX_BIT (1 << 20) // NOTE: this is returned in edx for leaf NX_CPUID_LEAF

#define EFER_REGISTER_N 0xC0000080
#define NXE_BIT (1 << 11) // NOTE: enable for in EFER, not support bit

extern bool pae_on;
extern bool nx_on;

void cpu_init(void);

#endif
