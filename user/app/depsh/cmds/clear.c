#include "app/depsh/cmds/clear.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"

void cmd_clear(char *args)
{
  if (!streq(args, ""))
  {
    write_string("\nclear: option ");
    write_string(args);
    write_string(" does not exist\n");
    return;
  }

  clear_screen();
}
