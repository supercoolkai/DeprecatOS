#include "drivers/fb/fbController.h"
#include "util/kprintf/kprintf.h"
#include "memory/mmap/memoryMap.h"
#include "memory/paging/paging.h"
#include "memory/heap/kernelHeap.h"

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

  FBChar blank;
  blank.c = ' ';
  blank.color = 0;

  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      shadow[r * cols + c] = blank;
    }
  }
}

static void draw_cursor(void)
{
  uint32_t color_val = palette[WHITE];

  for(int r = 0; r < GLYPH_HEIGHT; r++){
    for (int b = 0; b < GLYPH_WIDTH; b++) {
      uint32_t *line = (uint32_t *)(base + (row * GLYPH_HEIGHT + r) * pitch);

      line[col * GLYPH_WIDTH + b] = color_val;
    }
  }

  cursor_col = col;
  cursor_row = row;
}


static void draw_char(unsigned char c, unsigned char color)
{
  if (color >= PALETTE_SIZE) 
    return;

  uint32_t color_val = palette[color];

  const uint8_t *glyph = &font8x16[c * GLYPH_HEIGHT];

  for (int r = 0; r < GLYPH_HEIGHT; r ++) {
    for (int b = 0; b < GLYPH_WIDTH; b++) {
      int val = (glyph[r] >> (7 - b)) & 1;
      uint32_t *line = (uint32_t *)(base + (row * GLYPH_HEIGHT + r) * pitch);

      if (!val) {
        line[col * GLYPH_WIDTH + b] = palette[BLACK];
        continue;
      }
      
      line[col * GLYPH_WIDTH + b] = color_val;
    }
  }

  shadow[row * cols + col].c = c;
  shadow[row * cols + col].color = color;
}

static void del_cursor(void)
{
  int backup_row = row;
  int backup_col = col;

  col = cursor_col;
  row = cursor_row;

  FBChar curr = shadow[row * cols + col];
  draw_char(curr.c, curr.color);

  col = backup_col;
  row = backup_row;
}


static void scroll(void)
{
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

  for (row = 0; row < rows; row++){
    for (col = 0; col < cols; col++) {
      FBChar curr = shadow[row * cols + col];
      draw_char(curr.c, curr.color);
    }
  }

  col = 0;
  row--;

  del_cursor();
  draw_cursor();
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
