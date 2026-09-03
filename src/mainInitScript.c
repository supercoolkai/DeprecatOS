#include "drivers/timer/timerController.h"
#include "drivers/serial/serialController.h"
#include "drivers/fb/fbController.h"
#include "memory/mmap/memoryMap.h"
#include "drivers/keyboard/keyboard.h"
#include "exceptions/exceptions.h"
#include "memory/frameAlloc/frameAllocator.h"
#include "memory/paging/paging.h"
#include "memory/heap/kernelHeap.h"
#include "process/scheduler/scheduler.h"
#include "process/cpu/tssController.h"
#include "userland/syscall/syscallController.h"
#include "userland/userland.h"
#include "drivers/disk/ata.h"
#include "fs/block/blockController.h"

void mainInitScript(MBIInfo *info)
{
  // controller init
  timer_init();

  serial_init();

  exceptions_init();

  frame_alloc_init(info);
  paging_init();
  heap_init();
  

  keyboard_init();

  fb_init(info);

  scheduler_init();

  ata_init();

  block_init();

  tss_init();
  syscall_init();

  userland_init();

  while (1)
  {
    __asm__ volatile ("hlt");
  }
}
