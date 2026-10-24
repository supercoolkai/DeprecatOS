#include "app/depsh/cmds/umount.h"
#include "errors.h"
#include "sys/syscall.h"
#include "streq/streq.h"
#include <stdint.h>


void cmd_umount(char *args)
{
  if (streq(args, "")) {
    write_string("umount: must provide a fs to mount\n");
    return;
  }


  if (umount(args) == SYSCALL_ERROR) {
    write_string("umount: already unmounted or invalid path given\n");
    return;
  }
}
