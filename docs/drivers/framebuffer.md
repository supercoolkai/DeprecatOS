# Frame Buffer 

## FBChar struct format
src `src/drivers/fb/fbController.h`

one text cell with color and text bundled

| off | field | type |
|---|---|---|
| 0 | `c` | `unsigned char` |
| 1 | `color` | `unsigned char` |

---

## color palette format

src: `src/drivers/fb/fbController.h`

| name | value |
|---|---|
| `BLACK` | 0 |
| `BLUE` | 1 |
| `GREEN` | 2 |
| `CYAN` | 3 |
| `RED` | 4 |
| `MAGENTA` | 5 |
| `ORANGE` | 6 |
| `GRAY` | 7 |
| `DARK_GRAY` | 8 |
| `LIGHT_BLUE` | 9 |
| `LIGHT_GREEN` | 10 |
| `LIGHT_CYAN` | 11 |
| `LIGHT_RED` | 12 | 
| `LIGHT_MAGENTA` | 13 |
| `YELLOW` | 14 |
| `WHITE` | 15 |

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

#### `static FBChar *history`:
a storage of all history lines in scope (defined by `HISTORY_LINES`), used for scrolling up

#### `static int head`:
the top row of `shadow` relative to `history`, used in `scroll()`

#### `static int count`:
the total number of rows in `history`

#### `static int view_offset`:
the current offset where `shadow` is relative to `history`, `0 `being present and, for example, `5` being 5 rows up.

#### `static bool scrollback_on`:
a global `bool` deciding whether or not scrolling back should be on. settable with `fb_set_scrollback()`

## overview
a more versatile replacement for VGA, manually drawing each pixel based off of a bitmap. contains scroll support and a solid white cursor.

## function analysis 

### `void fb_init(MBIInfo *info)`
initializes the framebuffer (first ensures hardware compatibility) by checking the `MBIInfo *info`, which contains information about the framebuffer. sets all the global variables up and `kmalloc()`s space for `shadow`. also zeroes the framebuffer with `memset()`

### `static void draw_cursor(void)`
draws the cursor at the current `col` and `row`, and archives the position in `cursor_col` and `cursor_row`

### `static void blit_glyph(int r, int c, unsigned char ch, unsigned char color`
copies the char `ch` onto the framebuffer in color `color`, row `r` and col `c`. does not update `shadow` or the `cursor_row`/`cursor_col`

### `static void draw_char_forced(unsigned char c, unsigned char color)`
draws the given char `c` from the bitmap in color `color` using `blit_glyph()`, updating `shadow` as well

### `static void draw_char(unsigned char c, unsigned char color)`
draws the character `c` in color `color` by calling `draw_char_forced()` with these as parameters. first validates to see whether the `shadow` already has the char at the current `row`/`col`

### `static void del_cursor(void)`
deletes the cursor stored at (`cursor_col`, `cursor_row`)

### `static void scroll(void)`
scrolls the whole framebuffer using `memcpy()`, copying `shadow` into `history` at offset `head`, then updates `head` to be incremented (as long as it's below `HISTORY_LINES`). does the same with `count`. then manually copies all the `FBChar`s in `shadow` up. finally `memmove()`s `base` to move all framebuffer lines (except for top line, which gets erased) up by one row, and `memset`s the bottom row to be all zeroes. Finally calls `draw_cursor()`

### `static void view_repaint(void)`
repaints the view by finding the logical row in `history`. if the current logical row is in bounds, get the current row from `history`, otherwise get it from `shadow`. then `blit_glyph()`s all columns in the found row. essentially scrolls the framebuffer up by one row.

### `static void snap_to_live(void)`
snaps to the shadow, overwriting the entire screen with whatever's on `shadow`

### `void fb_scrollback(int delta)`
gets the new offset by adding `view_offset` with `delta`, then checks if it's in bounds. if it's out of bounds, change it to their respective leftmost/rightmost boundary (`0` or `count`). then, if the new offset is `0`, then calls `snap_to_live()`. otherwise, calls `view_repaint()`. essentially if `delta` is negative, scrolls it down once, and if it's positive, scrolls it up once. nothing happens if `delta == 0` OR `scrollback_on` is `false`

### `static void upd_row(void)`
adds 1 to `row`. if it exceeds `rows` then call `scroll()`.

### `static void upd_col(void)`
adds 1 to `col`. if it exceeds `cols` then call `upd_row()`

### `void fb_draw_char_upd(unsigned char c, unsigned char color)`
handler for real users. first, checks whether `view_offset` is not zero, and if so set it to the present and `snap_to_live()`. handles special keys like `\n` and `\b`. if its neither of those, it calls `draw_char()` with parameters `c` and `color`, and calls `upd_col()`. for special keys, this is what it does:
 - `\b`: if `col` is greater than 0, then move it back once. then, delete the character which sits at that position.
 - `\n`: call `upd_row()` and set `col` to 0

### `void fb_draw_char_no_upd(unsigned char c, unsigned char color`
essentially a 1:1 replica of `fb_draw_char_upd()`, but doesnt touch the cursor

### `void fb_draw_string(const char *str, unsigned char color)`
for each character in the given string, call `fb_draw_char_upd(c, color)`, where `c` is the current character. 

### `void fb_draw_string_no_upd(const char *str, unsigned char color)`
essentially a 1:1 replica of `fb_draw_string()`, but calls `fb_draw_char_no_upd()` instead of `fb_draw_char_upd()`

### `void fb_get_screen_dims(uint32_t *rows_out, uint32_t *cols_out)`
gives `cols` and `rows` to the out parameters

### `bool fb_set_cursor(uint32_t in_row, uint32_t in_col)`
first, if `view_offset` is not zero then set it to the present and `snap_to_live()`. then, check if the given coordinates are out of bounds, if so return `false`. then, sets `row` and `col` to the given parameters, and updates the cursor

### `bool fb_set_cursor_no_upd(uint32_t in_row, uint32_t in_col)`
essentially a 1:1 replica of `fb_set_cursor()`, but doesn't update the cursor

### `void fb_get_cursor(uint32_t *row_out, uint32_t *col_out)`
gives `row` and `col` to the out params

### `void fb_clear_screen(void)`
clears the entire screen, first calls `snap_to_live()` if applicable, and blanks the entire shadow and framebuffer. finally sets `row` and `col` to `0`, and updates the cursor

### `void fb_set_scrollback(bool b)`
sets `scrollback_on` to whatever `b` is
