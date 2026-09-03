#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <stdint.h>

typedef struct 
{
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint32_t sym0;
  uint32_t sym1;
  uint32_t sym2;
  uint32_t sym3;
  uint32_t mmap_length;
  uint32_t mmap_addr;
  uint32_t drives_length;
  uint32_t drives_addr;
  uint32_t config_table;
  uint32_t boot_loader_name;
  uint32_t apm_table;
  uint32_t vbe_control_info;
  uint32_t vbe_mode_info;
  uint32_t vbe_mode_seg;
  uint32_t vbe_off_len;
  uint32_t framebuffer_addrLo;
  uint32_t framebuffer_addrHi;
  uint32_t framebuffer_pitch;
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
  uint8_t  framebuffer_bpp;
  uint8_t  framebuffer_type;
} MBIInfo;

typedef struct
{
  uint32_t size;
  uint32_t baseLo;
  uint32_t baseHi;
  uint32_t lenLo;
  uint32_t lenHi;
  uint32_t type;
} MemoryMapEntry;

void mmap_print(const MBIInfo *info);
void mmap_walk(const MBIInfo *info, void (*fn)(uint32_t base, uint32_t len, uint32_t type));

#endif
