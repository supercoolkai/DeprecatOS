# dinv

## mode enum format
| name | purpose |
| --- | --- |
| `MODE_NORMAL` | switched on to be normal mode (navigation) |
| `MODE_INSERT` | switched on to be insert mode (editing) |
| `MODE_COMMAND` | switched on to be command mode (commands) |

## DinvCommand struct format
| offset | field | type |
| --- | --- | --- |
| 0 | `name` | `char *` |
| 4 | `fn` | `void(*fn)(void)` |

## damage enum format
| name | purpose |
| --- | --- |
| `DMG_NONE` | means no change has been made, no framebuffer refresh needed |
| `DMG_TAIL` | means there has been change to the current line, only refresh the line |
| `DMG_CMD` | means there's been change to the COMMAND line, only refresh that line |
| `DMG_BELOW` | means that there's been damage that impacts everything including the current line and below, refresh that part |
| `DMG_FULL` | means that the whole screen has been damaged, refresh the whole screen |

## global variables
#### `static char blank_header[COL_OFFSET+1]`:
`COL_OFFSET` (6) spaces, written in place of a line number header on screen rows past the end of the text

#### `static uint32_t screen_rows`, `static uint32_t screen_cols`:
the screen dimensions from `get_screen_dims()`, re-read on every launch rather than cached once so a resolution change between runs is picked up

#### `static uint32_t text_rows`:
`screen_rows - 1`, the bottom row is reserved for the mode / command line

#### `static uint32_t top_row`, `static uint32_t left_col`:
the scroll position: the text row shown on screen row 0 and the text column shown right after the line number header. moved by `check_scroll()` whenever the cursor leaves the visible window

#### `static uint32_t mode`:
one of `MODE_NORMAL`, `MODE_INSERT`, `MODE_COMMAND`. picks which `handle_*()` gets the next key and what the bottom row says

#### `static bool running`:
the main loop condition, cleared by `quit()` / `save_quit()`

#### `static char text[TEXT_CAP]`:
the file contents as a gap buffer (`TEXT_CAP` is 262144 bytes). the file is loaded into the front, the gap sits at the cursor, and anything after the cursor lives at the tail

#### `static uint32_t gap_start`, `static uint32_t gap_end`:
the gap's bounds inside `text[]`. `gap_start` is the cursor (and the byte count before it), `gap_end` is the first byte after the cursor. empty file = `0` / `TEXT_CAP`. moving the cursor shuffles one byte across the gap

#### `static char cmd_buf[64]`, `static uint32_t cmd_len`:
what's been typed after `:` in command mode, not NUL-terminated until enter is pressed

#### `static char path[256]`:
the absolute path of the file being edited, copied out of `return_path()`'s buffer at launch

#### `static uint32_t replacement_status`:
how much of the screen `gap_render()` needs to redraw this frame: `DMG_NONE`, `DMG_TAIL` (the cursor's line), `DMG_BELOW` (the line above the cursor if there is one, the cursor's line, and everything under it), `DMG_FULL` (everything), `DMG_CMD` (the bottom row only). reset to `DMG_NONE` before each key is handled, set to `DMG_FULL` by `check_scroll()` when it scrolls

#### `static char *args_save`:
the raw `args` the shell passed in, kept so `save()` can hand it to `cmd_touch()` when the file doesn't exist yet

#### `static char linebuf[256]`:
scratch for `gap_render()`, one screen line is assembled here before being written with `write_string_len_no_upd()`

#### `static char complaint[64]`, `static bool complaining`, `static uint64_t complaint_expiry`:
the error message shown on the bottom row in place of the mode name, whether it's currently showing, and the tick at which it stops (set 5000 ticks ahead). cleared from the main loop while idle

#### `static DinvCommand cmds[]`, `static uint32_t cmd_cnt`:
the `:` command table (`q`, `w`, `wq`, `qa`, `wa`, `wqa`, the `a` forms are aliases since there's only ever one buffer) and its length

## overview
a simplistic modal text editor similar to vi. has smart(er) screen handling for better performance

## function analysis

### `static uint32_t buf_rows(void)`
get the number of rows in the `text` buffer and return it

### `static char sanitize(char c)`
given a character, if it is a byte which is unrenderable or is taken as a command in the framebuffer, replace it with `.`

### `static uint32_t text_cols(void)`
get the number of columns in `linebuf`, caps out at `screen_cols - COL_OFFSET`

### `static void cursor_right(void)`
moves the `text` buffer right once

### `static void cursor_left(void)`
moves the `text` buffer left once

### `static void quit(void)`
sets `running` to false and resets the scrollback to `true`, so that on the next cycle of the `while(running)` loop it breaks

### `static void save(void)`
saves the `text` buffer into the actual file. if it does not exist, create it using `cmd_touch()` THEN save. walks the cursor to the end of the buffer first so the text is contiguous, so after saving the cursor sits at end of file. complains "Failed to save" if the file still can't be resolved or `fsave()` fails

### `static void save_quit(void)`
save, then quit using their respective functions. if `save()` raised a complaint ("Failed to save") it does NOT quit, you stay in the editor with the complaint on the bottom row

### `static void cursor_rowcol(uint32_t *row, uint32_t *col)`
returns the cursor (relative to the `text` buffer)'s current row and column into the two output variables

### `static void check_scroll(void)`
handles whether or not the current damage caused a scroll (meaning the whole screen must be regenerated) and saves the check to `replacement_status`

### `static void write_line_header(uint32_t row)`
writes the line header in the given `row`. for clarification the line header is the line number, in the format "[LINE_N] > "

### `static void gap_render(void)`
renders the gap onto the screen, mainly uses a switch and regenerates the screen according to the `replacement_status`

### `static void handle_command(unsigned char c)`
handles the command case, first turns the `mode` to `MODE_COMMAND` and then adds the given character to the command line. (if it is '\b' it goes backwards, and if it is '\n' it checks whether the current `cmd_buf` is a valid command. if not found warn and either way reset `cmd_buf`/`cmd_len`, set `mode` to `MODE_NORMAL` and return). also goes to `MODE_NORMAL` if esc is hit any time

### `static void handle_insert(unsigned char c)`
handles the insert case, first turns the `mode` to `MODE_INSERT` and just adds the given `c` to the `text` buffer, shrinking the gap and setting the old start to `c`. unless `c` is esc, then it just moves to `MODE_NORMAL` and returns before any of that stuff happens. '\b' deletes the byte before the cursor instead (pulls `gap_start` back one, damage is `DMG_BELOW` if that byte was a newline, else `DMG_TAIL`). bytes >= 0x80 (the `CTRL_KEY_*` codes) are ignored, and if the gap is full the character is dropped

### `static void handle_normal(unsigned char c)`
handles the normal case, first turns the `mode` to `MODE_NORMAL` and just checks for the navigation letters (hjkl). unless the given `c` is either ':' or 'i' or 'a', then it switches mode to `MODE_COMMAND`, `MODE_INSERT`, or `MODE_INSERT` (after advancing the cursor to the right once) respectively.

### `void app_dinv(char *args)`
the main loop, sets up all the global variables, loads the file, and has the main `while(running)` loop which overall just reads the next inputted character, check whether it's not the `SENTINEL`, and sends it over to the `mode`'s respective handler. after that, calls `check_scroll()`  and `gap_render()`. when the loop finally breaks, it clears the screen and returns to return to a clean command prompt
