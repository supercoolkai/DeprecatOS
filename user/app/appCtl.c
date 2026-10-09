#include "app/appCtl.h"
#include "app/dinv/dinv.h"
#include "app/depsh/depsh.h"
#include <stdint.h>

App apps[] = {
  {"dinv", app_dinv},
  
  // SHELL CMDS
  {"help", cmd_help},
  {"echo", cmd_echo},
  {"ticks", cmd_ticks},
  {"cat", cmd_cat},
  {"ls", cmd_ls},
  {"cd", cmd_cd},
  {"stat", cmd_stat},
  {"mkdir", cmd_mkdir},
  {"rm", cmd_rm},
  {"clear", cmd_clear},
  {"touch", cmd_touch},
  {"date", cmd_date},
  {"mv", cmd_mv},
};

uint32_t app_cnt = sizeof(apps) / sizeof(apps[0]);
