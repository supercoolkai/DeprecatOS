# Memory Map

## overview
a collection of structs and functions to get information about and interact with the memory map from multiboot.

## function analysis

### `static void print_entry(uint32_t base, uint32_t len, uint32_t type)`
simply prints each of the parameters (`base`, `len`, and `type`) in that order. a printing wrapper for `mmap_walk()`

### `void mmap_print(const MBIINfo *info)`
calls `mmap_walk()` with `print_entry()` as the given function.

### `void mmap_walk(const MBIInfo *info, void (*fn)(uint32_t base, uint32_t len, uint32_t type))`
tests the info's `flags` first to see if they're valid, then for each `MemoryMapEntry *entry` in the given `MBIInfo *info`, run the function `fn` with the information from `*entry` as parameters.  
