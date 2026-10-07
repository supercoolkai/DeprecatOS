#include "drivers/fb/fbController.h"
#include "kprintf/kprintf.h"
#include "memory/mmap/memoryMap.h"
#include "memory/paging/paging.h"
#include "memory/heap/kernelHeap.h"
#include "mem/mem.h"

#define HISTORY_LINES 256

extern uint8_t font8x16[];

static const uint32_t palette[PALETTE_SIZE] = 
{
    0x000000,
    0x0000AA,
    0x00AA00,
    0x00AAAA,
    0xAA0000,
    0xAA00AA,
    0xAA5500,
    0xAAAAAA,
    0x555555,
    0x5555FF,
    0x55FF55,
    0x55FFFF,
    0xFF5555,
    0xFF55FF,
    0xFFFF55,
    0xFFFFFF,
};

static int col;
static int row;
static int cursor_col;
static int cursor_row;
static int cols;
static int rows;

static uint32_t base;
static uint32_t pitch;
static uint32_t width;
static uint32_t height;

static FBChar *shadow;
static FBChar *history;
static int head;
static int count;
static int view_offset;

static bool scrollback_on;

void fb_init(MBIInfo *info)
{
  // guards for hardware compatibility 
  if (!((info->flags) & (1 << 12))) {
    kprintf(KPRINTF_RED "KERNEL PANIC: INCOMPATIBLE HARDWARE" KPRINTF_RESET);
    for (;;)
      __asm__ volatile("hlt");
  }
  if(info->framebuffer_type != 1){
    kprintf(KPRINTF_RED "KERNEL PANIC: INCOMPATIBLE HARDWARE" KPRINTF_RESET);
    for (;;)
      __asm__ volatile("hlt");
  }

  if (info->framebuffer_addrHi != 0) {
    kprintf(KPRINTF_RED "KERNEL PANIC: INCOMPATIBLE HARDWARE" KPRINTF_RESET);
    for (;;)
      __asm__ volatile("hlt");
  }

  if (info->framebuffer_bpp != 32){
    kprintf(KPRINTF_RED "KERNEL PANIC: INCOMPATIBLE HARDWARE" KPRINTF_RESET);
    for (;;)
      __asm__ volatile("hlt");
  }
  
  // identity mapping
  uint32_t start = info->framebuffer_addrLo & ~0xFFFu;
  uint32_t end_no_round = info->framebuffer_addrLo + info->framebuffer_pitch * info->framebuffer_height;
  uint32_t end = (end_no_round + 0xFFF) & ~0xFFFu;

  for (uint32_t i = start; i < end; i += 0x1000){
    map_kernel_page(i, i, PAGE_PRESENT | PAGE_RW);
  }

  base = info->framebuffer_addrLo;
  pitch = info->framebuffer_pitch;
  width = info->framebuffer_width;
  height = info->framebuffer_height;

  cols = width/GLYPH_WIDTH;
  //rows = height/16;
  rows = height/GLYPH_HEIGHT;

  col = 0;
  row = 0;

  cursor_col = 0;
  cursor_row = 0;

  shadow = kmalloc(rows * cols * sizeof(FBChar));
  history = kmalloc(HISTORY_LINES * cols * sizeof(FBChar));
  head = 0;
  count = 0;
  view_offset = 0;
  scrollback_on = true;

  FBChar blank;
  blank.c = ' ';
  blank.color = 0;

  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      shadow[r * cols + c] = blank;
    }
  }

  memset((uint8_t *) base, 0, pitch * height);
}

static void draw_cursor(void)
{
  uint32_t color_val = palette[WHITE];

  for(int r = 0; r < GLYPH_HEIGHT; r++){
    uint32_t *line = (uint32_t *)(base + (row * GLYPH_HEIGHT + r) * pitch);
    for (int b = 0; b < GLYPH_WIDTH; b++) {

      line[col * GLYPH_WIDTH + b] = color_val;
    }
  }

  if (shadow[row * cols + col].c != ' ') {
    const uint8_t *glyph = &font8x16[shadow[row*cols+col].c * GLYPH_HEIGHT];
    for (int r = 0; r < GLYPH_HEIGHT; r++) {
      uint32_t *line = (uint32_t *)(base + (row * GLYPH_HEIGHT + r) * pitch);
      for (int b = 0; b < GLYPH_WIDTH; b++) {
        int val = (glyph[r] >> (7 - b)) & 1;
        if (val) 
          line[col * GLYPH_WIDTH + b] = palette[BLACK];
      }
    }
  }

  cursor_col = col;
  cursor_row = row;
}


static void blit_glyph(int r, int c, unsigned char ch, unsigned char color)
{
  if (color >= PALETTE_SIZE) 
    return;

  uint32_t color_val = palette[color];

  const uint8_t *glyph = &font8x16[ch * GLYPH_HEIGHT];

  for (int i = 0; i < GLYPH_HEIGHT; i ++) {
    uint32_t *line = (uint32_t *)(base + (r * GLYPH_HEIGHT + i) * pitch);
    for (int b = 0; b < GLYPH_WIDTH; b++) {
      int val = (glyph[i] >> (7 - b)) & 1;

      if (!val) {
        line[c * GLYPH_WIDTH + b] = palette[BLACK];
        continue;
      }
      
      line[c * GLYPH_WIDTH + b] = color_val;
    }
  }
}

static void draw_char_forced(unsigned char c, unsigned char color)
{
  if (color >= PALETTE_SIZE) 
    return;

  blit_glyph(row, col, c, color);

  shadow[row * cols + col].c = c;
  shadow[row * cols + col].color = color;
}

static void draw_char(unsigned char c, unsigned char color)
{
  FBChar curr = shadow[row * cols + col];

  if (curr.c == c && curr.color == color)
    return;

  draw_char_forced(c, color);
}

static void del_cursor(void)
{
  int backup_row = row;
  int backup_col = col;

  col = cursor_col;
  row = cursor_row;

  FBChar curr = shadow[row * cols + col];
  draw_char_forced(curr.c, curr.color);

  col = backup_col;
  row = backup_row;
}


static void scroll(void)
{
  del_cursor();

  memcpy(&history[head*cols], &shadow[0 * cols], cols * sizeof(FBChar));
  head = (head + 1) % HISTORY_LINES;
  if (count < HISTORY_LINES)
    count++;

  for (int r = 1; r < rows; r++) {
    for (int c = 0; c < cols; c++){
      FBChar curr = shadow[r * cols + c];
      shadow[(r-1) * cols + c].c = curr.c;
      shadow[(r-1) * cols + c].color = curr.color;
    }
  }
  
  FBChar blank;
  blank.c = ' ';
  blank.color = 0;
  for (int c = 0; c < cols; c++) {
    shadow[(rows-1) * cols + c].c = blank.c;
    shadow[(rows-1) * cols + c].color = blank.color;
  }
  
  row = rows-1;
  col = 0;

  memmove((uint8_t *) base,
         (const uint8_t *) base + GLYPH_HEIGHT * pitch,
         (rows - 1) * GLYPH_HEIGHT * pitch);

  memset((uint8_t *) base + (rows - 1) * GLYPH_HEIGHT * pitch, 
         0, 
         GLYPH_HEIGHT * pitch);

  draw_cursor();
}

static void view_repaint(void)
{
  for (int r = 0; r < rows; r++){
    int logical = count + r - view_offset;

    FBChar *src;
    if (logical < count)
      src = &history[((head - count + logical + HISTORY_LINES) % HISTORY_LINES) * cols];
    else
      src = &shadow[(logical - count) * cols];

    for (int c = 0; c < cols; c++)
      blit_glyph(r, c, src[c].c, src[c].color);
  }
}

static void snap_to_live(void)
{
  for (int r = 0; r < rows; r++){
    for (int c = 0; c < cols; c++) {
      FBChar cell = shadow[r * cols + c];
      blit_glyph(r, c, cell.c, cell.color);
    }
  }
  draw_cursor();
}

void fb_scrollback(int delta)
{
  if (!scrollback_on)
    return;
  int off = view_offset + delta;

  if (off > count) off = count;
  if (off < 0) off = 0;
  if (off == view_offset) return;
  view_offset = off;
  if (view_offset == 0) {
    snap_to_live();
    return;
  }
  view_repaint();
}

static void upd_row(void)
{
  row ++;

  if(row < rows)
    return;

  scroll();
}

static void upd_col(void)
{
  col++;

  if (col < cols) 
    return;

  col = 0;

  upd_row();
}

void fb_draw_char_upd(unsigned char c, unsigned char color)
{
  if (view_offset) {
    view_offset = 0;
    snap_to_live();
  }
  if (c != '\b' && c != '\n'){
    draw_char(c, color);
    upd_col();
  }

  else if (c != '\n') {
    if (col > 0){ 
      col --;
      draw_char(' ', 0);
    }
  }

  else{
    upd_row();
    col = 0;
  }

  del_cursor();
  draw_cursor();
}

void fb_draw_char_no_upd(unsigned char c, unsigned char color)
{
  if (view_offset) {
    view_offset = 0;
    snap_to_live();
  }
  if (c != '\b' && c != '\n'){
    draw_char(c, color);
    upd_col();
  }

  else if (c != '\n') {
    if (col > 0){ 
      col --;
      draw_char(' ', 0);
    }
  }

  else{
    upd_row();
    col = 0;
  }
}

void fb_draw_string(const char *str, unsigned char color)
{
  for(int i = 0; i >= 0; i++) {
    unsigned char c = str[i];

    if(c == '\0') {
      return;
    }

    fb_draw_char_upd(c, color);
  }
}

void fb_draw_string_no_upd(const char *str, unsigned char color)
{
  for(int i = 0; i >= 0; i++) {
    unsigned char c = str[i];

    if(c == '\0') {
      return;
    }

    fb_draw_char_no_upd(c, color);
  }
}

void fb_get_screen_dims(uint32_t *rows_out, uint32_t *cols_out)
{
  *cols_out = cols;
  *rows_out = rows;
}

bool fb_set_cursor(uint32_t in_row, uint32_t in_col)
{
  if (view_offset) {
    view_offset = 0;
    snap_to_live();
  }
  if (in_row >= rows || in_col >= cols) 
    return false;

  row = in_row;
  col = in_col;
  del_cursor();
  draw_cursor();

  return true;
}

bool fb_set_cursor_no_upd(uint32_t in_row, uint32_t in_col)
{
  if (view_offset) {
    view_offset = 0;
    snap_to_live();
  }
  if (in_row >= rows || in_col >= cols) 
    return false;

  row = in_row;
  col = in_col;

  return true;
}

void fb_get_cursor(uint32_t *row_out, uint32_t *col_out)
{
  *row_out = row;
  *col_out = col;
}

void fb_clear_screen(void)
{
  if (view_offset) {
    view_offset = 0;
    snap_to_live();
  }
  FBChar blank;
  blank.c = ' ';
  blank.color = 0;
  for (int r = 0; r < rows; r++){
    for (int c = 0; c < cols; c++) {   
      row = r;
      col = c;

      draw_char(blank.c, blank.color);
    }
  }

  row = 0;
  col = 0;

  del_cursor();
  draw_cursor();
}

void fb_set_scrollback(bool b)
{
  scrollback_on = b;
}
