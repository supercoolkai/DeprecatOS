#include "strcat/strcat.h"
#include "strlen/strlen.h"
#include <stdint.h>

char *strcat(char *a, const char *b)
{
  uint32_t a_len = strlen(a);
  uint32_t i = 0;

  while (b[i] != '\0') {
    a[a_len + i] = b[i];
    i++;
  }

  a[a_len + i] = '\0';

  return a;
}

char *strncat(char *a, const char *b, uint32_t overflow)
{
  uint32_t a_len = strlen(a);
  uint32_t b_len = strlen(b);

  if (a_len >= overflow || a_len + b_len >= overflow)
    return 0;

  return strcat(a, b);
}
