# Shared libraries

## overview
This is a list of libraries usable in both kernel and userland alongside a description of them (no ring barriers, no port IO)

More complex libraries get their own .md files

## libraries

### `colors` (`etc/lib/colors/colors.h`)
`BLACK` 0, `BLUE` 1, `GREEN` 2, `CYAN` 3, `RED` 4, `MAGENTA` 5, `ORANGE` 6, `GRAY` 7, `DARK_GRAY` 8, `LIGHT_BLUE` 9, `LIGHT_GREEN` 10, `LIGHT_CYAN` 11, `LIGHT_RED` 12, `LIGHT_MAGENTA` 13, `YELLOW` 14, `WHITE` 15
a collection of `#define`s for the framebuffer color palette

### `epochToDate` (`etc/lib/epochToDate/epochToDate.h`)
`void epoch_to_date(uint64_t epoch, uint64_t *year_out, uint64_t *month_out, uint64_t *day_out, uint64_t *hour_out, uint64_t *minute_out, uint64_t *second_out, uint64_t *millisecond_out)`
Uses a formula to convert an epoch to an organized date

### `keys` (`etc/lib/keys/ctrlkeys.h`)
`CTRL_KEY_UP` 0x80, `CTRL_KEY_DOWN` 0x81, `CTRL_KEY_LEFT` 0x82, `CTRL_KEY_RIGHT` 0x83, `CTRL_KEY_HOME` 0x84, `CTRL_KEY_END` 0x85, `CTRL_KEY_DELETE` 0x86, `CTRL_KEY_PGUP` 0x87, `CTRL_KEY_PGDN` 0x88
a collection of `#define`s for the shell to know which keys dont have an ASCII character

### `longops` (`etc/lib/longops/longops.h`)
`uint64_t __udivmoddi4(uint64_t n, uint64_t d, uint64_t *rem)`

`uint64_t __udivdi3(uint64_t n, uint64_t d)`

`uint64_t __umoddi3(uint64_t n, uint64_t d)`

A manual replication of `uint64_t` operations because of the fact that x86-32 doesn't inherently have 64 bit operations

### `max` (`etc/lib/max/max.h`)
`uint32_t max(uint32_t a, uint32_t b)`
a simplistic max operator returning the maximum value of the two params

### `mem` (`etc/lib/mem/mem.h`)
`void *memcpy(void *dst, const void *src, size_t n)`

`void *memmove(void *dst, const void *src, size_t n)`

`void *memset(void *dst, int c, size_t n)`

`int memcmp(const void *a, const void *b, size_t n)`

A manual replication of clib's mem operations

### `min` (`etc/lib/min/min.h`)
`uint32_t min(uint32_t a, uint32_t b)`
A simplistic min operator, returning the minimum value of the two params

### `rb` (`etc/lib/rb/ringBuffer.h`)
see `docs/etc/lib/ringBuffer.md`

### `strcat` (`etc/lib/strcat/strcat.h`)
`char *strcat(char *a, const char *b)`

`char *strncat(char *a, const char *b, uint32_t overflow)`

A manual replication of clib's strcat

### `streq` (`etc/lib/streq/streq.h`)
`int streq(const char *a, const char *b)`

A manual replication of clib's strcmp, but only checks for equality

### `strlen` (`etc/lib/strlen/strlen.h`)
`#define MAX_COMP_LEN 1000000000`

`uint32_t strlen(const char *s)`

A manual replication of clib's strlen
