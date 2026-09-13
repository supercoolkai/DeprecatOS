#include "util/streq/streq.h"

int streq(const char *a, const char *b)
{
  int i = 0;

  while (a[i] == b[i])
  {
    if (a[i] == 0)
      return 1;

    i++;
  }

  return 0;
}
