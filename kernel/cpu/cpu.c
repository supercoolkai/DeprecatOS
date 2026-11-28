#include "cpu/cpu.h"

bool pae_on;
bool nx_on;

static void cpuid(uint32_t leaf_n, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx){
  __asm__ volatile ("cpuid" :
    "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) :
    "a"(leaf_n), "c"(0));
}

static void read_efer(uint32_t *eax, uint32_t *edx)
{
  __asm__ volatile ("rdmsr" :
    "=a" (*eax), "=d" (*edx) :
    "c" (EFER_REGISTER_N)
  );
}

static void write_efer(uint32_t eax, uint32_t edx)
{
  __asm__ volatile ("wrmsr" ::
    "a"(eax), "c" (EFER_REGISTER_N), "d" (edx)
  );
}

void cpu_init(void)
{
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;

  cpuid(PREFERRED_CPUID_LEAF, &eax, &ebx, &ecx, &edx);

  pae_on = (bool)(edx & PAE_BIT);
  nx_on = false;
  
  // check for NX support
  if (pae_on) {
    cpuid(HIGH_CPUID_LEAF_TEST, &eax, &ebx, &ecx, &edx);

    uint32_t max_leaf = eax;

    if (max_leaf >= NX_CPUID_LEAF){
      cpuid(NX_CPUID_LEAF, &eax, &ebx, &ecx, &edx);

      if (!(edx & NX_BIT))
        return;

      nx_on = true;

      // now we turn on
      uint32_t efer_lo;
      uint32_t efer_hi;
      read_efer(&efer_lo, &efer_hi);

      efer_lo |= NXE_BIT;

      write_efer(efer_lo, efer_hi);
    }
  }
}
