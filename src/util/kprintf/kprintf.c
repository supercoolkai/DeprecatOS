#include "util/kprintf/kprintf.h"
#include "drivers/fb/fbController.h"
#include "drivers/serial/serialController.h"
#include "util/streq/streq.h"
#include <stdint.h>
#include <stdbool.h>

#define PALETTE_LENGTH 8
#define IS_PRINTING_FLAG 1
#define BUFFER_SIZE 65536

typedef struct {
  const char *esc_clr;
  unsigned char fb_clr;
} PrintfColor;

static PrintfColor esc_clr_palette_foregrnd[] = {
  {"30", BLACK},
  {"31", RED},
  {"32", GREEN},
  {"33", YELLOW},
  {"34", BLUE},
  {"35", MAGENTA},
  {"36", CYAN},
  {"37", WHITE},
  {"39", WHITE},
};

static bool good_to_fb = false;
static FBChar buf[BUFFER_SIZE / sizeof(FBChar)];
static int curr_buf_ind = 0;

void kprintf_init(void)
{
  good_to_fb = true;
  FBChar curr;

  for (int i = 0; i < curr_buf_ind; i++) {
    curr = buf[i];
    fb_draw_char_upd(curr.c, curr.color);
  }
}

void kprintf(const char *str)
{
  unsigned char clr = WHITE;
  char c;
  int i = 0;
  uint8_t status_flags = 0;
  while ((c = str[i]) != 0)
  {

    if (c == ESC_PREFIX[0]) {
      char seq[ESC_SUFFIX_LEN];

      seq[0] = str[i+1];
    
      for (int j = 1; j < ESC_SUFFIX_LEN; j++){
        if (str[i+j] == 0){
          if (good_to_fb) {
            fb_draw_string("\nkprintf: WARNING (malformed escape sequence, length err), returning early!\n", YELLOW);
          }
          serial_write_string("\nkprintf: WARNING (malformed escape sequence, length err), returning early!\n");
          return;
        }

        seq[j] = str[i+j];
      }

      i += ESC_SUFFIX_LEN;

      if (seq[1] == '3' && seq[2] != '8') { // (excluding 38/48), 30-39 = foreground, 40-49 = backgroud
        if (seq[2] == '9'){
          status_flags &= ~IS_PRINTING_FLAG;
          clr = WHITE;
          continue;
        }

        char clr_seq[ESC_SUFFIX_LEN];
        clr_seq[ESC_SUFFIX_LEN - 1] = 0;

        for (int j = 1; j < ESC_SUFFIX_LEN; j++){
          clr_seq[j-1] = seq[j];
        }

        for (int j = 0; j < PALETTE_LENGTH; j++) {
          PrintfColor curr_clr = esc_clr_palette_foregrnd[j];
          if (streq(clr_seq, curr_clr.esc_clr)) {
            clr = curr_clr.fb_clr;
            break;
          }
        }

        status_flags |= IS_PRINTING_FLAG;
        continue;
      }

      else{
        if (good_to_fb){
          fb_draw_string("\nkprintf: WARNING (malformed escape sequence, invalid seq), returning early!\n", YELLOW);
        }
        serial_write_string("\nkprintf: WARNING (malformed escape sequence, invalid seq), returning early!\n");
        return;
      }
    }

    else if (status_flags & IS_PRINTING_FLAG) {
      if (good_to_fb) {
        fb_draw_char_upd((unsigned char) c, clr);
      }
    
      if (curr_buf_ind < (BUFFER_SIZE / sizeof(FBChar))){ 
        buf[curr_buf_ind] = (FBChar) {c, clr}; 
        curr_buf_ind++;
      }

      serial_write_char(c);
    }

    i++;
  }
}
