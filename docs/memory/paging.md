# Paging

## data structure tables / related information
src: `src/memory/paging/paging.h`, `src/memory/heap/kernelHeap.c`, `src/userland/userland.h`

### flag bits:
**page-entry flag bits** (passed to `map_kernel_page`):

| macro | bit | meaning |
|---|---|---|
| `PAGE_PRESENT` | `0x1` | entry is valid |
| `PAGE_RW` | `0x2` | writable |
| `PAGE_USER` | `0x4` | ring-3 accessible |

### PDE (page descriptor entry) format:

| bits | field |
|---|---|
| 0 | present |
| 1 | read/write |
| 2 | user/supervisor |
| 3 | write-through |
| 4 | cache-disable |
| 5 | accessed |
| 6 | dirty (PTE) |
| 7 | page-size (PDE) / PAT (PTE) |
| 8 | global |
| 9–11 | available for OS |
| 12–31 | physical frame address (4 KiB-aligned) |

## global variables
#### `static uint32_t *kernel_dir`:
an address to the frame of the kernel directory (which is assigned in `paging_init()`)

## overview
manages permissions for memory. essentially attaches permissions to frames.

## function analysis
### `static void map_page(uint32_t *dir, uint32_t virt, uint32_t phys, uint32_t flags)`
first checks whether `dir[di]` is present (`di` being `virt >> 22`), if not then allocate a new frame for it and add it to `dir` with flags `PRESENT | RW`. then, adds `phys` to `table`'s entry at `virt >> 12`'s last 10 digits (`table` being `dir[di]` rounded to the nearest frame)

### `void map_kernel_page(uint32_t virt, uint32_t phys, uint32_t flags)`
just calls `map_page()` with the given parameters but with parameter `dir` being passed as `kernel_dir`

### `uint32_t unmap_kernel_page(uint32_t virt)`
with the given `virt`, deduce the `di` and, if present, clear the table entry, then run `invlpg (%0)` with `virt` as `r` in order to clear any cached address from the cpu

### `void paging_init(void)`
calls `alloc_frame()` to setup `kernel_dir`, and from memory address `0x1000` to `0x400000` map a page for `kernel_dir` with flags `PAGE_PRESENT | PAGE_RW`
