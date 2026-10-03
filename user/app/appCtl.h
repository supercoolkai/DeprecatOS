#ifndef APPCTL_H
#define APPCTL_H

#include <stdint.h>

typedef struct {
  char *name;
  void (*fn)(char *args);
} App;

extern App apps[];
extern uint32_t app_cnt;

#endif
