# Heap Controller

## data structure tables / related information

### heap block header

src: `src/memory/heap/kernelHeap.c`  `BlockHeader` (8 bytes, not asserted)

bump/first-fit allocator each block is `[BlockHeader][payload]`. `kmalloc` rounds request up to a multiple of 4 and splits when the remainder exceeds a header.

| off | field | type | notes |
|---|---|---|---|
| 0 | `size` | `uint32_t` | payload bytes (excludes header) |
| 4 | `is_free` | `uint32_t` | 1 = free, 0 = in use |

---

## global variables
#### `static uint32_t heap_start`:
the address of where the heap starts

#### `static uint32_t heap_end`:
the address of where the heap ends

## overview
a distributor of dynamic retractable chunks of memory. currently only supports `kmalloc()`, free `kmalloc()`s with `kfree()`

## function analysis

### `void heap_init(void)`
assigns `heap_start` and `heap_end`, as well as calls `map_kernel_page()` at `heap_start`. also creates the seed `BlockHeader`

### `void *kmalloc(uint32_t size)`
allocates a block (the start of which denoted by a `BlockHeader`) with a size greater than or equal to `size`. once found, if its large enough to split into two, split the block into whatever is occupied and the rest of the space left. 

### `void kfree(void *ptr)`
frees the block at `ptr`, and coalesces every free block ahead of it to combine with the now-free block
