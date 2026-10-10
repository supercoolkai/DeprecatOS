#include "app/depsh/cmds/ln.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "errors.h"
#include <stdint.h>


void cmd_ln(char *args)
{
  // NOTE: leave, symlinks r next and flag is the next thing we gon do
  char *flag = "";
  char *other_args = args;

  if (args[0] == '-'){
    flag = args;
    int i = 0;
    while (args[i] != ' ' && args[i] != 0)
      i++;

    if (args[i] == ' ') {
      args[i] = 0;
      other_args = args + i + 1;
    }
    else {
      other_args = args + i;
    }
  }
  
  char *path = other_args;
  char *link_name = other_args;
  int i = 0;
  while (other_args[i] != ' ' && other_args[i] != 0)
    i++;

  if (other_args[i] == ' ') {
    other_args[i] = 0;
    link_name = other_args + i + 1;
  }
  else {
    link_name = other_args + i;
  }

  if (streq(link_name, "") || streq(path, "")){
    write_string("ln: invalid arguments.\n");
    return;
  }

  char *absolute_og_path = (char *) return_path(path);
  
  if (absolute_og_path == 0){
    write_string("ln: path too long\n");
    return;
  }
  
  uint32_t inode_n;
  if ((inode_n = resolve_dir(absolute_og_path)) == SYSCALL_ERROR){
    write_string("ln: cannot make a link to a nonexistent file\n");
    return;
  }

  if (is_dir(inode_n)) {
    write_string("ln: cannot make a link to a directory\n");
    return;
  }
  
  char *absolute_new_path = (char *) return_path(link_name);

  if (absolute_new_path == 0) {
    write_string("ln: path for the link is too long\n");
    return;
  }

  if (resolve_dir(absolute_new_path) != SYSCALL_ERROR) {
    write_string("ln: given link path already exists\n");
    return;
  }

  uint32_t link_parent_n;
  char *link_leaf;

  if (!split_parent_leaf(absolute_new_path, &link_parent_n, &link_leaf)) {
    write_string("ln: given link path's parent does not exist\n");
    return;
  }

  if (lmake_hard(inode_n, link_parent_n, link_leaf) == SYSCALL_ERROR) {
    write_string("ln: unexpected error occured while making the link\n");
  }
}
