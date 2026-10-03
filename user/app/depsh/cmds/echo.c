#include "app/depsh/cmds/echo.h"
#include "sys/syscall.h"

void cmd_echo(char *args)
{
  write_string(args);
  write_string("\n");
}
