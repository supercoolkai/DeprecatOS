#ifndef FBCONTROLLER_H
#define FBCONTROLLER_H
#define BLACK 0
#define GREEN 2
#define RED 4
#define YELLOW 14
#define WHITE 15

#include "memory/mmap/memoryMap.h"

typedef struct {
  unsigned char c;
  unsigned char color;
} FBChar;

void fb_init(MBIInfo *info);
void fb_draw_char_upd(unsigned char c, unsigned char color);
void fb_draw_string(const char *str, unsigned char color);

#endif
