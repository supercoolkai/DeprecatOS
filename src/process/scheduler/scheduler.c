#include "process/scheduler/scheduler.h"
#include "process/queue/processQueue.h"
#include "memory/heap/kernelHeap.h"
#include "process/cpu/tssController.h"
#include "drivers/fb/fbController.h"
#include "memory/paging/paging.h"
#include "util/hex/hexPrinter.h"
#include "userland/userland.h"
#include "drivers/serial/serialController.h"
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
  
  p->esp = p->malloc_addr + STACK_SIZE - 44;

  uint32_t *frame = (uint32_t *)p->esp;

  for (int i = 0; i < 8; i ++)
    frame[i] = 0;
  
  frame[8] = (uint32_t) entry;
  frame[9] = 0x08;
  frame[10] = 0x202;
  
  pqueue_push(p);
}

void create_user_process(uint32_t entry, uint32_t user_stack_top)
{
  Process *p = create_proc_no_forgery();

  if(p == NULL)
    return;
 
  p->esp = p->malloc_addr + STACK_SIZE - 52;

  uint32_t *frame = (uint32_t *)p->esp;

  for (int i = 0; i < 8; i ++)
    frame[i] = 0;
  
  frame[8] = entry;
  frame[9] = 0x1B;
  frame[10] = 0x202;
  frame[11] = user_stack_top;
  frame[12] = 0x23;
  
  pqueue_push(p);
}

uint32_t kill_current(void)
{
  serial_write_string("\nATTEMPTING PKILL\n");
  fb_draw_string("\nATTEMPTING PKILL\n", YELLOW);

  if (pqueue_is_empty()){
    serial_write_string("\nEPIC PKILL FAIL\n");
    fb_draw_string("\nEPIC PKILL FAIL\n", RED);
    kfree((void *)0x00000000);
    return 0xFFFFFFFF; // No bueno but this prolly wont happen so its ok
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
