#include "app/depsh/cmds/mount.h"
#include "errors.h"
#include "sys/syscall.h"
#include "streq/streq.h"
#include <stdint.h>


void cmd_mount(char *args)
{
  if (streq(args, "")) {
    write_string("mount: must provide a fs to umount\n");
    return;
  }


  if (mount(args) == SYSCALL_ERROR) {
    write_string("mount: already mounted or invalid path given\n");
    return;
  }
}
