#include "cpu/cpu.h"

bool pae_on;

static void cpuid(uint32_t leaf_n, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx){
  __asm__ volatile ("cpuid" :
    "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) :
    "a"(leaf_n), "c"(0));
}

void cpu_init(void)
{
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;

  cpuid(PREFERRED_CPUID_LEAF, &eax, &ebx, &ecx, &edx);

  pae_on = (bool)(edx & PAE_BIT);
}
