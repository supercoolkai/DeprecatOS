#ifndef RINGBUFFER_H
#define RINGBUFFER_H
#include <stdbool.h>

typedef struct 
{
  volatile unsigned char queue[256];
  volatile unsigned char head;
  volatile unsigned char tail;
} ringBuffer;

extern ringBuffer g_rb;

void rb_init(ringBuffer *rb);
bool rb_is_empty(const ringBuffer *rb);
bool rb_push(ringBuffer *rb, unsigned char c);
bool rb_pop(ringBuffer *rb, unsigned char *out);
bool rb_peek(const ringBuffer *rb, unsigned char *out);

#endif
