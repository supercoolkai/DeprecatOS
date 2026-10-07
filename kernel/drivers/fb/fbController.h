#ifndef FBCONTROLLER_H
#define FBCONTROLLER_H

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
void fb_get_screen_dims(uint32_t *rows_out, uint32_t *cols_out);
bool fb_set_cursor(uint32_t in_row, uint32_t in_col);
bool fb_set_cursor_no_upd(uint32_t in_row, uint32_t in_col);
void fb_draw_char_no_upd(unsigned char c, unsigned char color);
void fb_draw_string_no_upd(const char *str, unsigned char color);
void fb_clear_screen(void);
void fb_scrollback(int delta);
void fb_set_scrollback(bool b);
void fb_get_cursor(uint32_t *row_out, uint32_t *col_out);

#endif
