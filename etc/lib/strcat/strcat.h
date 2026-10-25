#ifndef STRCAT_H
#define STRCAT_H

#include <stdint.h>

char *strcat(char *a, const char *b);
char *strncat(char *a, const char *b, uint32_t overflow);

#endif
