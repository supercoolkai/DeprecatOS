#include "util/hex/hexPrinter.h"
#include <stdint.h>
#include "drivers/fb/fbController.h"
#include "drivers/serial/serialController.h"


void print_hex(uint32_t v, unsigned char clr)
{
  for (int i=7; i >= 0; i--) {
    uint8_t nib = (v >> (i*4)) & 0xF; 
    char c = "0123456789ABCDEF"[nib];
   
    serial_write_char(c);
    fb_draw_char_upd(c, clr);
  }
}
