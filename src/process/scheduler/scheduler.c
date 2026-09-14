#include "process/scheduler/scheduler.h"
#include "util/kprintf/kprintf.h"
#include "process/queue/processQueue.h"
#include "memory/heap/kernelHeap.h"
#include "gdt/tssController.h"
#include "drivers/fb/fbController.h"
#include "memory/paging/paging.h"
#include "util/hex/hexPrinter.h"
#include "userland/userland.h"
#include "drivers/serial/serialController.h"
#include "gdt/segments.h"
#include "errors.h"
#include <stddef.h>

static Process *current;

void scheduler_init(void)
{
  Process *p = kmalloc(sizeof(Process));
  p->malloc_addr = 0;
  current = p;
}

uint32_t schedule(uint32_t esp)
{
  if(pqueue_is_empty())
    return esp;

  current->esp = esp;
  pqueue_push(current);
  current = pqueue_pop();

  if(current->malloc_addr != 0)
    tss_set_esp(current->malloc_addr + STACK_SIZE); 
  return current->esp;
}

static Process *create_proc_no_forgery(void)
{
  Process *p = kmalloc(sizeof(Process));

  if (!p)
    return NULL;
  
  uint32_t stackAlloc = (uint32_t) kmalloc(STACK_SIZE);

  if(!stackAlloc)
    return NULL;

  p->malloc_addr = stackAlloc;

  return p;
}

void create_process(void (*entry)(void))
{
  Process *p = create_proc_no_forgery();

  if(p == NULL)
    return;
  
  p->esp = p->malloc_addr + STACK_SIZE - KERNEL_FRAME_SIZE;

  uint32_t *frame = (uint32_t *)p->esp;

  // Btw the reason these are zeroed
  // is because this is for the general 
  // purpose registers, but cuz
  // its a brand new process it has no history 
  // so everything should be zeroed
  for (int i = 0; i < 8; i ++)
    frame[i] = 0;
  
  frame[FRAME_EIP_INDEX] = (uint32_t) entry;
  frame[FRAME_CS_INDEX] = KERNEL_CODE_SEGMENT_SEL;
  frame[FRAME_EFLAGS_INDEX] = INTERRUPT_AND_RESERVED_EFLAGS;
  
  pqueue_push(p);
}

void create_user_process(uint32_t entry, uint32_t user_stack_top)
{
  Process *p = create_proc_no_forgery();

  if(p == NULL)
    return;
 
  p->esp = p->malloc_addr + STACK_SIZE - USER_FRAME_SIZE;

  uint32_t *frame = (uint32_t *)p->esp;
  
  // Btw the reason these are zeroed
  // is because this is for the general 
  // purpose registers, but cuz
  // its a brand new process it has no history 
  // so everything should be zeroed
  for (int i = 0; i < 8; i ++)
    frame[i] = 0;
  
  frame[FRAME_EIP_INDEX] = entry;
  frame[FRAME_CS_INDEX] = USER_CODE_SEGMENT_SEL;
  frame[FRAME_EFLAGS_INDEX] = INTERRUPT_AND_RESERVED_EFLAGS;
  frame[FRAME_USER_ESP_INDEX] = user_stack_top;
  frame[FRAME_SS_INDEX] = USER_DATA_SEGMENT_SEL;
  
  pqueue_push(p);
}

uint32_t kill_current(void)
{
  kprintf(KPRINTF_YELLOW "\nATTEMPTING PKILL\n" KPRINTF_RESET);

  if (pqueue_is_empty()){
    kprintf(KPRINTF_RED "\nEPIC PKILL FAIL\n" KPRINTF_RESET);
    kfree((void *)0x00000000);
    return SYSCALL_ERROR; // No bueno but this prolly wont happen so its ok
  }

  Process *victim = current; 

  Process *next = pqueue_pop();

  current = next;



  if(current->malloc_addr != 0)
    tss_set_esp(current->malloc_addr + STACK_SIZE);

  userland_teardown();

  kfree((void *) (victim->malloc_addr));

  kfree((void *) victim);

  return current->esp;
}
