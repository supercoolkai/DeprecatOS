#include "util/rb/ringBuffer.h"

void rb_init(ringBuffer *rb)
{
  rb->tail = 0;
  rb->head = 0;
}

bool rb_is_empty(const ringBuffer *rb)
{
  if (rb->tail == rb->head)
    return true;

  return false;
}

bool rb_push(ringBuffer *rb, unsigned char c)
{
  unsigned char next = rb->head+1;
  if (next == rb->tail)
    return false;
  
  rb->queue[rb->head] = c;
  rb->head = next;
  
  return true;
}

bool rb_pop(ringBuffer *rb, unsigned char *out)
{
  if(rb_is_empty(rb))
    return false;

  *out = rb->queue[rb->tail];
  rb->tail++;
  return true;
}

bool rb_peek(const ringBuffer *rb, unsigned char *out)
{
  if(rb_is_empty(rb))
    return false;
  *out = rb->queue[rb->tail];
  return true;
}
