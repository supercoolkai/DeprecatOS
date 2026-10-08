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
  
  uint64_t n;
  get_ticks(&n);
  print_uint64(n);
  write_string("\n");
}
