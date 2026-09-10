# Frame Allocator

## global variables
#### `static uint32_t frame_bitmap[32768]`:
a bitmap of all the frames (4 GiB of frames, 32 * 32768 frames because 32 bits per entry)

#### `extern char _ebss[]`:
not a real array, but the address of where the kernel image ends and free memory starts (what this gives out)

## overview
the thing that hands out frames of memory, used generally anywhere but typically in `paging`. 4 GiB of frames available in total (the same amt of memory that a 32-bit CPU can use)

## function overview

### `static void mark_free_region(uint32_t base, uint32_t len, uint32_t type)`
marks the given frames in the region as free in the bitmap (0)

### `void frame_alloc_init(const MBIInfo *info)`
initializes the frame allocator. calls `mmap_walk()` to mark all of the memory as free. 

### `uint32_t alloc_frame(void)`
allocates the next frame from the bitmap to be occupied and return the address of that. if no more frames are available, return 0.

### `void free_frame(uint32_t addr)`
marks the bitmap entry of `addr` as free.
