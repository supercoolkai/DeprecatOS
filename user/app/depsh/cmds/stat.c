#include "app/depsh/cmds/stat.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "errors.h"
#include <stdint.h>

void cmd_stat(char *args)
{
  char *flag = "";
  char *path_arg = args;

  if (args[0] == '-') {
    flag = args;

    int i = 0;
    while (args[i] != ' ' && args[i] != 0)
      i++;

    if (args[i] == ' ') {
      args[i] = 0;
      path_arg = args + i + 1;
    }
    else {
      path_arg = args + i;
    }
  }

  if (streq(flag, "")){
    write_string("stat: no flag provided.\n");
    return;
  }

  if (streq(path_arg, "")){
    write_string("stat: no file/dir provided.\n");
    return;
  }

  const char *absolute_path = return_path(path_arg);

  if (absolute_path == 0){
    write_string("stat: invalid path\n");
    return;
  }

  uint32_t inode_n = resolve_dir(absolute_path);

  if (inode_n == SYSCALL_ERROR) {
    write_string("stat: file ");
    write_string((char *) absolute_path);
    write_string(" does not exist or is not a file\n");
    return;
  }

  get_stat(inode_n, (uint32_t *) stat_buf);

  uint32_t return_val;
  if (streq(flag, "-s") || streq(flag, "--size")) {
    return_val = *(uint32_t*)(stat_buf + 4);
  }
  else if(streq(flag, "-t") || streq(flag, "--type")) {
    return_val = (uint32_t)(*(uint16_t *)(stat_buf + 0));
  }
  else if(streq(flag, "-l") || streq(flag, "--links")) {
    return_val = (uint32_t)(*(uint16_t*)(stat_buf + 26));
  }
  else{
    write_string("stat: invalid flag.\n");
    return;
  }

  print_uint32(return_val);
  write_char('\n');
}
