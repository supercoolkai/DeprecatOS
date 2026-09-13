#include "memory/frameAlloc/frameAllocator.h"
#include "util/kprintf/kprintf.h"
#include "userland/userland.h"
#include "memory/paging/paging.h"
#include "process/scheduler/scheduler.h"
#include "drivers/fb/fbController.h"
#include "drivers/serial/serialController.h"
#include <stdint.h>

static uint32_t frames[MISC_PAGE_CNT];

static void map_misc_pages(void)
{
  for (int i = 0; i < MISC_PAGE_CNT; i++) {
    uint32_t curr_frame = alloc_frame();

    if (!curr_frame){
      kprintf(KPRINTF_YELLOW "alloc_frame() failed in a map_misc_pages()'s misc frame init!\n" KPRINTF_RESET);
      for (int j = 0; j < i; j++) {
        free_frame(frames[j]);
        frames[j] = 0;
      }

      return;
    }

    frames[i] = curr_frame;
  }

  for (int i = 0; i < MISC_PAGE_CNT; i++) {
    uint32_t addr = CODE_ADDR + 0x1000 * (i + 1);
    map_kernel_page(addr, frames[i], PAGE_PRESENT | PAGE_RW | PAGE_USER);
  }
}

void userland_init(void)
{
  uint32_t code_frame = alloc_frame();
  uint32_t stack_frame = alloc_frame();

  if (!code_frame){
    kprintf(KPRINTF_YELLOW "alloc_frame() failed in userland_init()'s code frame init!\n" KPRINTF_RESET);
    if (stack_frame != 0)
      free_frame(stack_frame);

    return;
  }

  if (!stack_frame){
    kprintf(KPRINTF_YELLOW "alloc_frame() failed in userland_init()'s stack frame init!\n" KPRINTF_RESET);
    free_frame(code_frame);
    return;
  }

  map_kernel_page(CODE_ADDR, code_frame, PAGE_PRESENT | PAGE_RW | PAGE_USER);
  map_misc_pages();
  map_kernel_page(STACK_ADDR, stack_frame, PAGE_PRESENT | PAGE_RW | PAGE_USER);
  
  

  extern char user_prog_start[];
  extern char user_prog_end[];

  uint32_t user_prog_bytes = user_prog_end - user_prog_start;

  char* destination = (char *) CODE_ADDR;

  for (uint32_t i = 0; i < user_prog_bytes; i++){
    destination[i] = user_prog_start[i];
  }

  kprintf(KPRINTF_WHITE "[ " KPRINTF_RESET KPRINTF_GREEN "OK" KPRINTF_RESET KPRINTF_WHITE " ]\n");

  kprintf(KPRINTF_GREEN "* " KPRINTF_RESET KPRINTF_WHITE "Welcome to " KPRINTF_RESET 
          KPRINTF_CYAN "DeprecatOS (Indev 0.01)!\n" KPRINTF_RESET);
  create_user_process(CODE_ADDR, STACK_ADDR + 0x1000);
}

void userland_teardown(void)
{
  uint32_t code_phys = unmap_kernel_page(CODE_ADDR);
  uint32_t stack_phys = unmap_kernel_page(STACK_ADDR);

  if (code_phys != 0)
    free_frame(code_phys);

  if (stack_phys != 0)
    free_frame(stack_phys);

  for (int j = 0; j < MISC_PAGE_CNT; j++) {
    if (frames[j] == 0) 
      continue;
    uint32_t addr = CODE_ADDR + 0x1000 * (j + 1);
    unmap_kernel_page(addr);

    free_frame(frames[j]);
  }
}
