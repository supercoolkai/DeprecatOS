#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

void exceptions_init(void);
void panic(char *msg) __attribute__((noreturn));

#endif
