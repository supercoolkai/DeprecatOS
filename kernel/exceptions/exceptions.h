#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#define FRAME_CS_INDEX_EXCEPTION 11
#define FRAME_VECTOR_INDEX_EXCEPTION 8
#define FRAME_ERROR_CODE_INDEX_EXCEPTION 9

#define PAGE_FAULT_VECTOR 14

void exceptions_init(void);
void panic(char *msg) __attribute__((noreturn));

#endif
