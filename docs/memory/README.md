# Memory Management

## structs

### `MBIInfo` in `src/memory/mmap/memoryMap.h`
contains overall info for the mmap, such as:
 - `uint32_t flags`: permissions and misc info about the mmap 
 - `uint32_t mmap_addr`: address of where the mmap is located
 - `uint32_t mmap_size`: size of the mmap in bytes

additionally contains framebuffer information like:
 - `uint32_t framebuffer_addrLo`: the low 4 bytes of the framebuffer address
 - `uint32_t framebuffer_addrHi`: the high 4 bytes of the framebuffer address
 - `uint32_t framebuffer_width`: the width of the framebuffer in bits or "pixels"
 - `uint32_t framebuffer_height`: the height of the framebuffer in bits or "pixels"

### `MemoryMapEntry` in `src/memory/mmap/memoryMap.h`
a basic entry for the mmap, used in `mmap_walk()` to represent a piece of memory representing a uint32_t, char, etc.

### `BlockHeader` in `src/memory/heap/kernelHeap.c`
a header representing a block (a dynamic amount of allocated memory), the fields being:
 - `uint32_t size`: the size of allocated memory (excluding the BlockHeader struct size)
 - `uint32_t is_free`: a "bool" representing whether or not it is free. 1 = free, 0 = used

## overview

### `memoryMap`:
manages the memory map, currently only used in the `frameAllocator` to mark everything as free in `frame_alloc_init`

### `frameAllocator`:
manages the frame allocation, allowing any other function to allocate a frame of memory (4096 bytes) out of all 1,048,576 frames (32 bit words, 32,768 words in bitmap). currently does not support allocating frames at arbitrary numbers.

### `paging`:
manages paging, currently only allows mapping kernel pages. takes a virtual memory address and a physical memory address, alongside flags which contain information about the page being mapped

### `heap`:
KERRently only supports kmallocs. overall attempts to find the first available BlockHeader which is large enough for that size and splits it into only as much memory as required (in order to prevent unused space)
