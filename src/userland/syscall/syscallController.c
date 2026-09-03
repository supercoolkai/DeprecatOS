#include "userland/syscall/syscallController.h"
#include <stdint.h>
#include "drivers/timer/timerController.h"
#include "drivers/serial/serialController.h"
#include "drivers/fb/fbController.h"
#include "memory/paging/paging.h"
#include "process/scheduler/scheduler.h"
#include "util/rb/ringBuffer.h"
#include "userland/userland.h"
#include "fs/ext2/directoryController.h"
#include "fs/block/blockController.h"
#include "fs/ext2/blockGroupDescriptor.h"
#include "fs/ext2/inode.h"

static uint32_t scratch[BIT_32_PER_BLK];

extern void syscall_stub(void);

static uint32_t sys_write_char(uint32_t *frame)
{
  serial_write_char((char)frame[4]);
  fb_draw_char_upd((char)frame[4], WHITE);

  frame[7] = 0;
  return (uint32_t) frame;
}

static uint32_t sys_write_string(uint32_t *frame)
{
  char *s = (char *) frame[4];
  
  uint32_t i = 0;

  for (;;){

    if((uint32_t) s + i < KERNEL_CEILING || (uint32_t) s + i >= USER_SPACE_END){
      frame[7] = 0xFFFFFFFF;
      return (uint32_t) frame;
    }
    char c = s[i];
    
    if (c == 0)
      break;

    serial_write_char(c);
    fb_draw_char_upd(c, WHITE);

    i++;
  }
  frame[7] = 0;
  return (uint32_t) frame;
}

static uint32_t sys_get_ticks(uint32_t *frame)
{
  frame[7] = timer_get_tick();
  return (uint32_t) frame;
}

static uint32_t sys_exit_curr(uint32_t *frame)
{
// CAUTION: this function is prone to error
  // due to kill_current() inherently
  // standing on the victim's stack

  uint32_t res = (uint32_t) kill_current();
  if (res == 0xFFFFFFFF){
    serial_write_string("\n\nKERNEL PANIC: TRIED TO EXIT WITH NO OTHER PROCESSES AVAILABLE");
    fb_draw_string("\n\nKERNEL PANIC: TRIED TO EXIT WITH NO OTHER PROCESSES AVAILABLE", RED);
    for (;;)
      __asm__ volatile ("hlt");
  }
  
  return res;
}

static uint32_t sys_read_char(uint32_t *frame)
{
  unsigned char c;
  

  if(rb_pop(&g_rb, &c))
  {
    frame[7] = c;
  }

  else{
    frame[7] = SENTINEL;
  }

  return (uint32_t) frame;
}

static uint32_t sys_yield(uint32_t *frame)
{
  frame[7] = 0;
  return schedule((uint32_t) frame);
}

static uint32_t sys_write_char_color(uint32_t *frame)
{
  unsigned char color = (unsigned char) frame[6];
  serial_write_char((char)frame[4]);
  fb_draw_char_upd((char)frame[4], color);

  frame[7] = 0;
  return (uint32_t) frame;
}

static uint32_t sys_write_string_color(uint32_t *frame)
{
  char *s = (char *) frame[4];
  unsigned char color = (unsigned char) frame[6];
  
  uint32_t i = 0;

  for (;;){

    if((uint32_t) s + i < KERNEL_CEILING || (uint32_t) s + i >= USER_SPACE_END){
      frame[7] = 0xFFFFFFFF;
      return (uint32_t) frame;
    }
    char c = s[i];
    
    if (c == 0)
      break;

    serial_write_char(c);
    fb_draw_char_upd(c, color);

    i++;
  }
  frame[7] = 0;
  return (uint32_t) frame;

}

static uint32_t resolve_dir(const char *s)
{  
  uint32_t i = 0;
  for (;;) {
    if ((uint32_t) s + i < KERNEL_CEILING || (uint32_t) s + i >= USER_SPACE_END)
      return 0xFFFFFFFF;

    char c = s[i];

    if (c == 0)
      break;

    i++;
  }
  
  uint32_t ino;
  bool success = lookup_path(s, &ino);

  if (!success) {
    return 0xFFFFFFFF;
  }

  return ino;
}

static uint32_t sys_read_chunk(uint32_t *frame)
{
  uint32_t ino_n = frame[4];
  struct ext2_inode ino;
  
  if (!get_inode(ino_n, &ino)) {
    frame[7] = 0xFFFFFFFF;
    return (uint32_t) frame;
  }

  uint32_t n = frame[5];
  uint32_t *buf = (uint32_t *) frame[6];

  if ((uint32_t) buf < KERNEL_CEILING || (uint32_t) buf > USER_SPACE_END - BLOCK_SIZE){
    frame[7] = 0xFFFFFFFF;
    return (uint32_t) frame;
  }
  
  uint32_t blocks = (ino.size_lo + BLOCK_SIZE - 1) / BLOCK_SIZE;
  if (n >= blocks) {
    frame[7] = 0;
    return (uint32_t) frame;
  }

  uint32_t block_n;

  if (n < 12) {
    block_n = ino.dir_block_ptr[n];
  }
  else if(n < BIT_32_PER_BLK + INODE_BLK_PTR_AMT){
    uint32_t m = n - INODE_BLK_PTR_AMT;
    read_block(ino.singly_indir_block_ptr, (uint16_t *) scratch);

    block_n = scratch[m];
  }
  else if (n < INODE_BLK_PTR_AMT + BIT_32_PER_BLK + (BIT_32_PER_BLK * BIT_32_PER_BLK)){
    uint32_t m = n - INODE_BLK_PTR_AMT - BIT_32_PER_BLK;
    read_block(ino.doubly_indir_block_ptr, (uint16_t *)scratch);

    uint32_t o = scratch[m / BIT_32_PER_BLK];
    read_block(o, (uint16_t *)scratch);

    block_n = scratch[m % BIT_32_PER_BLK];
  }

  else {
    uint32_t m = n - INODE_BLK_PTR_AMT - BIT_32_PER_BLK - (BIT_32_PER_BLK * BIT_32_PER_BLK);
    read_block(ino.triply_indir_block_ptr, (uint16_t *)scratch);

    uint32_t o = scratch[m / (BIT_32_PER_BLK * BIT_32_PER_BLK)];
    read_block(o, (uint16_t *)scratch);

    uint32_t p = scratch[(m / BIT_32_PER_BLK) % BIT_32_PER_BLK];
    read_block(p, (uint16_t *)scratch);

    block_n = scratch[m % BIT_32_PER_BLK];
  }

  if(block_n == 0) {
    frame[7] = 0xFFFFFFFF;
    return (uint32_t) frame;
  }
  
  read_block(block_n, (uint16_t *) buf);

  if (n == blocks - 1 && ino.size_lo % BLOCK_SIZE != 0){
    frame[7] = ino.size_lo % BLOCK_SIZE;
  }

  else{
    frame[7] = BLOCK_SIZE;
  }

  return (uint32_t) frame;
}

static uint32_t sys_resolve_dir(uint32_t *frame)
{
  const char *s = (const char *) frame[4];

  uint32_t ino = resolve_dir(s);

  frame[7] = ino;

  return (uint32_t) frame;
}

static uint32_t sys_write_string_len(uint32_t *frame)
{
  uint32_t base = frame[4];
  const char *s = (const char *) base;
  uint32_t len = frame[6];
  
  if (len > USER_SPACE_END - KERNEL_CEILING || base < KERNEL_CEILING || base > USER_SPACE_END - len) {
    frame[7] = 0xFFFFFFFF;
    return (uint32_t) frame;
  }

  for (uint32_t i = 0; i < len; i++){
    const char c = s[i];

    serial_write_char(c);
    fb_draw_char_upd(c, WHITE);
  }

  frame[7] = 0;
  return (uint32_t) frame;
}

static uint32_t sys_get_stat(uint32_t *frame)
{
  uint32_t ino_n = frame[4];
  struct ext2_inode ino;
  
  if (!get_inode(ino_n, &ino)) {
    frame[7] = 0xFFFFFFFF;
    return (uint32_t) frame;
  }

  uint32_t *buf = (uint32_t *) frame[6];

  if ((uint32_t) buf < KERNEL_CEILING || (uint32_t) buf > USER_SPACE_END - sizeof(struct ext2_inode)){
    frame[7] = 0xFFFFFFFF;
    return (uint32_t) frame;
  }
  
  *(struct ext2_inode *) buf = ino;
  frame[7] = 0;
  return (uint32_t) frame;
}

static uint32_t (*syscall_table[])(uint32_t *) = {
  sys_write_char,
  sys_write_string,
  sys_get_ticks,
  sys_exit_curr,
  sys_read_char,
  sys_yield,
  sys_write_char_color,
  sys_write_string_color,
  sys_read_chunk,
  sys_resolve_dir,
  sys_write_string_len,
  sys_get_stat, 
};

void syscall_init(void)
{
  idt_set_gate_type(SYSCALL_VECTOR_NUMBER, syscall_stub, 0xEE);
}

uint32_t syscall_handler(uint32_t esp)
{

  uint32_t *frame = (uint32_t *)esp;

  uint32_t eax = frame[7];
  
  int length = sizeof(syscall_table) / sizeof(syscall_table[0]);

  if(eax > length || eax < 1){
    frame[7] = 0xFFFFFFFF;
    return esp;
  }
  
  return syscall_table[eax - 1](frame);
}


