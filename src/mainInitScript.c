#include "idt/idtController.h"
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
#include "gdt/tssController.h"
#include "userland/syscall/syscallController.h"
#include "userland/userland.h"
#include "drivers/disk/ata.h"
#include "fs/block/blockController.h"
#include "fs/ext2/bitmapController.h"
#include "util/kprintf/kprintf.h"

void mainInitScript(MBIInfo *info)
{
  // controller init 

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the serial driver...           ");
  serial_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");
  
  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the IDT...                     ");
  idt_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the timer driver...            ");
  timer_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the exceptions handler...      ");
  exceptions_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the frame allocator...         ");
  frame_alloc_init(info);
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the paging controller...       ");
  paging_init(info);
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the heap...                    ");
  heap_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the keyboard...                ");
  keyboard_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  fb_init(info);
  kprintf_init();

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the scheduler...               ");
  scheduler_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the ATA driver...              ");
  ata_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the block driver...            ");
  block_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the ext2 bitmap layer...       ");
  ext2_bitmap_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the GDT and TSS controller...  ");
  tss_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the syscall controller...      ");
  syscall_init();
  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Initializing the userland...                ");
  userland_init();

  while (1)
  {
    __asm__ volatile ("hlt");
  }
}
