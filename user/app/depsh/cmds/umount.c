#include "app/depsh/cmds/umount.h"
#include "errors.h"
#include "sys/syscall.h"
#include "streq/streq.h"
#include "strcat/strcat.h"
#include "app/depsh/depshCommon.h"
#include "strlen/strlen.h"
#include <stdint.h>

void cmd_umount(char *args)
{
  if (streq(args, "")) {
    write_string("umount: must provide a fs to umount\n");
    return;
  }
  
  char path[32] = "/mnt/";
  
  char *args_mountpoint = strncat((char *) path, args, 32);
  if (!args_mountpoint){
    write_string("umount: something went wrong when trying to find the mountpoint\n");
    return;
  }
  uint32_t loop_len = strlen(args_mountpoint);

  if (loop_len <= strlen((char *) dir)){
    bool busy = true;
    for (uint32_t i = 0; i < loop_len; i++) {
      if (args_mountpoint[i] != dir[i]){
        busy = false;
        break;
      }
    }

    if (!(dir[loop_len] == '/' || !dir[loop_len]))
      busy = false;

    if (busy) {
      write_string("umount: busy dir\n");
      return;
    }
  }

  if (umount(args) == SYSCALL_ERROR) {
    write_string("umount: already unmounted or invalid path given\n");
    return;
  }
}
