#include "app/depsh/cmds/halt.h"
#include "sys/syscall.h"
#include "streq/streq.h"

void cmd_halt(char *args)
{
  if (!streq(args, "")){
    write_string("halt: too many arguments\n");
    return;
  }

  halt();
}
