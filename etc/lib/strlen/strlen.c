#include "strlen/strlen.h"
#include <stdint.h>

uint32_t strlen(const char *s)
{
  uint32_t i = 0;
  while(i < MAX_COMP_LEN) {
    if (s[i] == '\0')
      return i;

    i++;
  }

  return MAX_COMP_LEN + 1;
}
