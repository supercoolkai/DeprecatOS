#include "process/queue/processQueue.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

static Process *queue[256];
static unsigned char head;
static unsigned char tail;

void pqueue_init(void)
{
  head = 0;
  tail = 0;
}

bool pqueue_is_empty(void)
{
  if (head == tail)
    return true;

  return false;
}

Process *pqueue_pop(void)
{
  if(pqueue_is_empty()){
      return NULL;
  }
  unsigned char temp = tail;
  tail++;
  return queue[temp];
}

Process *pqueue_peek(void)
{
  if(pqueue_is_empty()){
    return NULL;
  }

  return queue[tail];
}

bool pqueue_push(Process *p)
{
  unsigned char next = head + 1;
  if (next == tail) {
      return false;
  }
  
  queue[head] = p;

  head = next;
  return true;
}

unsigned char pqueue_size(void)
{
  return (unsigned char) (head - tail);
}
