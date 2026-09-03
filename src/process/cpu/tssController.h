#ifndef TSSCONTROLLER_H
#define TSSCONTROLLER_H

#include <stdint.h>

typedef struct{
  uint32_t link; // filler as well
  uint32_t esp0;
  uint32_t ss0;
  uint32_t filler[22]; // words 3 -> 24 inclusive
  uint32_t IOBitmapOffset; // word 25
} tssInfo;
_Static_assert(sizeof(tssInfo) == 104, "TSS must be 104 bytes");

void tss_init(void);
void tss_set_esp(uint32_t n);

#endif
