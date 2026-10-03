#include "app/depsh/cmds/ticks.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"

void cmd_ticks(char *args)
{
  if (!streq(args, ""))
  {
    write_string("ticks: option ");
    write_string(args);
    write_string(" does not exist\n");
    return;
  }

  print_uint32(get_ticks());
  write_string("\n");
}
