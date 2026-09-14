#ifndef FBCONTROLLER_H
#define FBCONTROLLER_H
#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define ORANGE 6
#define GRAY 7
#define DARK_GRAY 8
#define LIGHT_BLUE 9
#define LIGHT_GREEN 10
#define LIGHT_CYAN 11
#define LIGHT_RED 12
#define LIGHT_MAGENTA 13
#define YELLOW 14
#define WHITE 15

#define GLYPH_WIDTH 8
#define GLYPH_HEIGHT 16
#define PALETTE_SIZE 16

#include "memory/mmap/memoryMap.h"

typedef struct {
  unsigned char c;
  unsigned char color;
} FBChar;

void fb_init(MBIInfo *info);
void fb_draw_char_upd(unsigned char c, unsigned char color);
void fb_draw_string(const char *str, unsigned char color);

#endif
