# kprintf

## PrintfColor struct format
src `etc/kernel/kprintf/kprintf.c` (private to it)

| off | field | type |
| --- | --- | --- |
| 0 | `esc_clr` | `const char *` |
| 4 | `fb_clr` | `unsigned char` |

## color palette format
src `etc/kernel/kprintf/kprintf.c`

| escape code | framebuffer color |
| --- | --- |
| `"30"` | `BLACK` |
| `"31"` | `RED` |
| `"32"` | `GREEN` |
| `"33"` | `YELLOW` |
| `"34"` | `BLUE` |
| `"35"` | `MAGENTA` |
| `"36"` | `CYAN` | 
| `"37"` | `WHITE` |
| `"39"` | `WHITE` |

## global variables
#### `static PrintfColor esc_clr_palette_foregrnd[]`:
a color palette of `PrintfColor` structs, much like the framebuffer color palette

#### `static bool good_to_fb`:
a `bool` which represents whether or not kprintf has been initialized yet, so all prints wait until it is initialized

#### `static FBChar buf[BUFFER_SIZE / sizeof(FBChar)]`:
a buffer of every character kprintf has printed, appended to on every call (before and after init) until it is full. flushed to the framebuffer exactly once, by `kprintf_init()`, so anything printed before `fb_init()` still shows up

#### `static int curr_buf_ind`:
just an index tracker of `buf`

## overview
a basic kprintf implementation, uses ANSI escape codes for colors and prints to both serial and framebuffer

## function analysis

### `void kprintf_init(void)`
sets `good_to_fb` to true and flushes the `buf`, must be called AFTER `fb_init()`

### `void kprintf(const char *str)`
prints the given string to both serial and the framebuffer. nothing is written anywhere until a color escape (`KPRINTF_WHITE` etc.) has been seen in that call, so a bare `kprintf("x")` prints nothing; `KPRINTF_RESET` turns printing back off. every printed char goes to serial, to `buf`, and (only if `good_to_fb`) to the framebuffer. a malformed escape sequence warns on both outputs and returns early 
