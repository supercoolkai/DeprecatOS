# Frame Buffer 

## global variables

#### `extern uint8_t font8x16[]`: 
the font bitmap from `src/drivers/disk/fb/font8x16.bin`, as the name says 8x16 bits per character.

#### `static const uint32_t palette[16]`:
a palette of 16 colors (taken from VGA) used mostly in `draw_char()`.

#### `static int col`:
the global column value (in characters, not bits)

#### `static int row`:
the global row value (in characters, not bits)

#### `static int cursor_col`:
the global cursor column value (in characters, not bits). used for recording where the cursor is to clean it up once it updates.

#### `static int cursor_row`:
the global cursor row value (in characters, not bits). used for recording where the cursor is to clean it up once it updates.

#### `static uint32_t base`:
the phys address of the framebuffer

#### `static uint32_t pitch`:
the **bytes** between the start of one scanline and the start of another.

#### `static uint32_t width`: 
how many pixels per row are visible on the screen

#### `static uint32_t width`:
the amount of pixels in each (1 pixel thick) row

#### `static uint32_t height`:
the amount of pixels in each (1 pixel wide) column

#### `static FBChar *shadow`:
a storage of all the characters currently on the screen, used by `scroll()` to move the whole screen up by one row.

## overview
a more versatile replacement for VGA, manually drawing each pixel based off of a bitmap. contains scroll support and a solid white cursor.

## function analysis 

### `void fb_init(MBIInfo *info)`
initializes the framebuffer (first ensures hardware compatibility) by checking the `MBIInfo *info`, which contains information about the framebuffer. sets all the global variables up and `kmalloc()`s space for `shadow`.

### `static void draw_cursor(void)`
draws the cursor at the current `col` and `row`, and archives the position in `cursor_col` and `cursor_row`

### `static void draw_char(unsigned char c, unsigned char color)`
draws the given char `c` from the bitmap in color `color`

### `static void del_cursor(void)`
deletes the cursor stored at (`cursor_col`, `cursor_row`)

### `static void scroll(void)`
scrolls the whole framebuffer using `shadow`, clearing the bottom row and moving each row up one. this pushes the top row off of the screen. currently no scrolling up.

### `static void upd_row(void)`
adds 1 to `row`. if it exceeds `rows` then call `scroll()`.

### `static void upd_col(void)`
adds 1 to `col`. if it exceeds `cols` then call `upd_row()`

### `void fb_draw_char_upd(unsigned char c, unsigned char color)`
handler for real users, handles special keys like `\n` and `\b`. if its neither of those, it calls `draw_char()` with parameters `c` and `color`, and calls `upd_col()`. for special keys, this is what it does:
 - `\b`: if `col` is greater than 0, then move it back once. then, delete the character which sits at that position.
 - `\n`: call `upd_row()` and set `col` to 0

### `void fb_string(const char *str, unsigned char color)`
for each character in the given string, call `fb_draw_char_upd(c, color)`, where `c` is the current character. 
