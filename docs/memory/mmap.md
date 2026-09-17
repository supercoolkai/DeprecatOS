# Memory Map

## data structure tables / related information

### MBInfo struct format

src: `src/memory/mmap/memoryMap.h`.

multiboot 1 info structure based off what GRUB hands. only fields currently read are framebuffer and mmap, add fields to the end of the struct if reading something else from it cuz this struct only has what the kernel currently needs, not everything the mmap hands

| off | field | notes |
|---|---|---|
| 0 | `flags` | which fields below are valid |
| 4 | `mem_lower` | KiB below 1 MiB |
| 8 | `mem_upper` | KiB above 1 MiB |
| 12 | `boot_device` | |
| 16 | `cmdline` | |
| 20 | `mods_count` | |
| 24 | `mods_addr` | |
| 28 | `sym0`–`sym3` | 16 B ELF/a.out symbol union |
| 44 | `mmap_length` | size of memory-map buffer |
| 48 | `mmap_addr` | pointer to first `MemoryMapEntry` |
| 52 | `drives_length` | |
| 56 | `drives_addr` | |
| 60 | `config_table` | |
| 64 | `boot_loader_name` | |
| 68 | `apm_table` | |
| 72 | `vbe_control_info` | |
| 76 | `vbe_mode_info` | |
| 80 | `vbe_mode_seg` | |
| 84 | `vbe_off_len` | |
| 88 | `framebuffer_addrLo` | linear framebuffer physical addr (low) |
| 92 | `framebuffer_addrHi` | (high) |
| 96 | `framebuffer_pitch` | bytes per scanline |
| 100 | `framebuffer_width` | |
| 104 | `framebuffer_height` | |
| 108 | `framebuffer_bpp` | `uint8_t` |
| 109 | `framebuffer_type` | `uint8_t` |

### MemoryMapEntry struct format

src: `src/memory/mmap/memoryMap.h`.

24 bytes, one entry per region at `mmap_addr`. `size` excludes the size of itself, so stride to the next entry is actually `base_of_entry + size + 4`

| off | field | notes |
|---|---|---|
| 0 | `size` | bytes in this entry beyond this field |
| 4 | `baseLo` | region base (low 32) |
| 8 | `baseHi` | region base (high 32) |
| 12 | `lenLo` | region length (low 32) |
| 16 | `lenHi` | region length (high 32) |
| 20 | `type` | 1 = usable RAM, else reserved |

---

## overview
a collection of structs and functions to get information about and interact with the memory map from multiboot.

## function analysis

### `static void print_entry(uint32_t base, uint32_t len, uint32_t type)`
simply prints each of the parameters (`base`, `len`, and `type`) in that order. a printing wrapper for `mmap_walk()`

### `void mmap_print(const MBIINfo *info)`
calls `mmap_walk()` with `print_entry()` as the given function.

### `void mmap_walk(const MBIInfo *info, void (*fn)(uint32_t base, uint32_t len, uint32_t type))`
tests the info's `flags` first to see if they're valid, then for each `MemoryMapEntry *entry` in the given `MBIInfo *info`, run the function `fn` with the information from `*entry` as parameters.  
