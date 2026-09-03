#include <stdint.h>
#include "memory/heap/kernelHeap.h"
#include "memory/paging/paging.h"
#include "memory/frameAlloc/frameAllocator.h"

typedef struct {
  uint32_t size;
  uint32_t is_free;
} BlockHeader;

static uint32_t heap_start;
static uint32_t heap_end;

void heap_init(void)
{
  map_kernel_page(0x400000, alloc_frame(), PAGE_PRESENT | PAGE_RW);

  heap_start = 0x400000;
  heap_end = 0x401000;

  BlockHeader *h = (BlockHeader*)heap_start;
  h->size = 4096 - sizeof(BlockHeader);
  h->is_free = 1;
}

void *kmalloc(uint32_t size)
{
  uint32_t wanted = (size + 3) & ~3u;
  BlockHeader *h; 

  for (uint32_t cursor = heap_start;
  cursor < heap_end; cursor +=
  sizeof(BlockHeader) + h->size) {
    h = (BlockHeader *)cursor;

    if (h->is_free && h->size >= wanted) {
      if(h->size - wanted > sizeof(BlockHeader)) {
        uint32_t new_addr = cursor + sizeof(BlockHeader) + wanted;
          
        BlockHeader *n = (BlockHeader *)new_addr;
        n->size = h->size - wanted - sizeof(BlockHeader);
        n->is_free = 1;
        h->size = wanted;
      }
    h->is_free = 0;
    return (void *)(h + 1);
    }
  }
  
  uint32_t frameAlloc = 0;

  if(heap_end < KERNEL_CEILING - 0x1000)
    frameAlloc = alloc_frame();

  if(frameAlloc != 0)
  {
    uint32_t growth = heap_end - heap_start;
    uint32_t old_heap_end = heap_end;

    heap_end += growth;

    for (uint32_t i = old_heap_end; i <= heap_end - 0x1000 && i <= KERNEL_CEILING - 0x1000; i += 0x1000){
      map_kernel_page(i, frameAlloc, PAGE_PRESENT | PAGE_RW);
      frameAlloc = alloc_frame();
      if(frameAlloc == 0){
        heap_end = i + 0x1000;
        
        break;
      }
    }

    if(heap_end > KERNEL_CEILING){
      heap_end = KERNEL_CEILING;
    }

    if (frameAlloc != 0) {
      free_frame(frameAlloc);
    }

    if(h->is_free){
      h->size += heap_end - old_heap_end;
    }
    else {
      BlockHeader *f = (BlockHeader *)old_heap_end;
      f->is_free = 1;
      f->size = (heap_end - old_heap_end) - sizeof(BlockHeader);
    }

    return kmalloc(size);
  }

  return 0;
}

void kfree(void *ptr)
{
  BlockHeader *h = (BlockHeader *) ptr - 1;

  h->is_free = 1;

  uint32_t cursor = ((uint32_t) ptr + h->size);
  BlockHeader *c = (BlockHeader *)cursor;
  
  
  while (cursor < heap_end && c->is_free) 
  {
    h->size += c->size + sizeof(BlockHeader);
    
    cursor = (uint32_t)ptr + h->size;

    c = (BlockHeader *)cursor;
  }
}
