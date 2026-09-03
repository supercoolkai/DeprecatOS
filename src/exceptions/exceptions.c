#include "exceptions/exceptions.h"
#include <stdint.h>
#include "drivers/serial/serialController.h"
#include "drivers/fb/fbController.h"
#include "drivers/timer/timerController.h"
#include "util/hex/hexPrinter.h"
#include "memory/paging/paging.h"
#include "process/scheduler/scheduler.h"

static const char *exc_names[32] = 
{
  "Divide Error",
  "Debug",
  "NMI",
  "Breakpoint",
  "Overflow",
  "BOUND Range Exceeded",
  "Invalid Opcode",
  "Device Not Available",
  "Double Fault",
  "Coprocessor Segment Overrun",
  "Invalid TSS",
  "Segment Not Present",
  "Stack-Segment Fault",
  "General Protection",
  "Page Fault",
  "Reserved",
  "x87 FP Exception",
  "Alignment Check",
  "Machine Check",
  "SIMD FP Exception",
  "Virtualization Exception",
  "Control Protection",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
};

extern void exc0(void);
extern void exc1(void);
extern void exc2(void);
extern void exc3(void);
extern void exc4(void);
extern void exc5(void);
extern void exc6(void);
extern void exc7(void);
extern void exc8(void);
extern void exc9(void);
extern void exc10 (void);
extern void exc11(void);
extern void exc12(void);
extern void exc13(void);
extern void exc14(void);
extern void exc15(void);
extern void exc16(void);
extern void exc17(void);
extern void exc18(void);
extern void exc19(void);
extern void exc20(void);
extern void exc21(void);
extern void exc22(void);
extern void exc23(void);
extern void exc24(void);
extern void exc25(void);
extern void exc26(void);
extern void exc27(void);
extern void exc28(void);
extern void exc29(void);
extern void exc30(void);
extern void exc31(void);

void exceptions_init(void)
{
  idt_set_gate(0, exc0);
  idt_set_gate(1, exc1);
  idt_set_gate(2, exc2);
  idt_set_gate(3, exc3);
  idt_set_gate(4, exc4);
  idt_set_gate(5, exc5);
  idt_set_gate(6, exc6);
  idt_set_gate(7, exc7);
  idt_set_gate(8, exc8);
  idt_set_gate(9, exc9);
  idt_set_gate(10, exc10);
  idt_set_gate(11, exc11);
  idt_set_gate(12, exc12);
  idt_set_gate(13, exc13);
  idt_set_gate(14, exc14);
  idt_set_gate(15, exc15);
  idt_set_gate(16, exc16);
  idt_set_gate(17, exc17);
  idt_set_gate(18, exc18);
  idt_set_gate(19, exc19);
  idt_set_gate(20, exc20);
  idt_set_gate(21, exc21);
  idt_set_gate(22, exc22);
  idt_set_gate(23, exc23);
  idt_set_gate(24, exc24);
  idt_set_gate(25, exc25);
  idt_set_gate(26, exc26);
  idt_set_gate(27, exc27);
  idt_set_gate(28, exc28);
  idt_set_gate(29, exc29);
  idt_set_gate(30, exc30);
  idt_set_gate(31, exc31);
}

void panic(char *msg) {
  fb_draw_char_upd('\n', RED);
  fb_draw_string(msg, RED);

  for (;;) __asm__ volatile ("hlt");
}

uint32_t exception_handler(uint32_t esp)
{

  uint32_t *frame = (uint32_t *)esp;

  if((frame[11] & 3) == 0) {
    uint32_t vector = frame[8];
    uint32_t error_code = frame[9];
  
    serial_write_string(exc_names[vector]);
    fb_draw_string(exc_names[vector], RED);

    serial_write_string(", ERROR CODE: ");
    fb_draw_string(", ERROR CODE: ", RED);

    print_hex(error_code, RED);
    serial_write_string("\n");
    fb_draw_string("\n", RED);

    if (vector == 14) {
      serial_write_string("CR2: ");
      fb_draw_string("CR2: ", RED);

      serial_write_char(read_cr2());
      print_hex(read_cr2(), RED);

      serial_write_string("\n");
      fb_draw_string("\n", RED);
    }

    for (;;) __asm__ volatile ("hlt");

  }

  if((frame[11] & 3) == 3) {
    uint32_t vector = frame[8];
    uint32_t error_code = frame[9];
  
    serial_write_string(exc_names[vector]);
    fb_draw_string(exc_names[vector], YELLOW);

    serial_write_string(", ERROR CODE: ");
    fb_draw_string(", ERROR CODE: ", YELLOW);

    print_hex(error_code, YELLOW);

    serial_write_string("\n");
    fb_draw_string("\n", YELLOW);

    serial_write_string("PROCESS KILLED");
    fb_draw_string("PROCESS KILLED", YELLOW);

    serial_write_string("\n");
    fb_draw_string("\n", YELLOW);

  }
  return kill_current();
}




