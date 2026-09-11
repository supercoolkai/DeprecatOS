# Hex Printer

## overview
a small utility to print a given `uint32_t` in hexadecimal form to the framebuffer and serial in a given color

## function analysis
### `void print_hex(uint32_t v, unsigned char clr)`

for each byte in `v`, print the hexadecimal value of the byte in serial and framebuffer, using `serial_write_char(c)` and `fb_draw_char_upd(c, clr)`
