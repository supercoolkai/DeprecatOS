#include "sys/syscall.h"
#include "drivers/fb/fbController.h"
#include "userland/syscall/syscallController.h"
#include "app/appCtl.h"
#include "streq/streq.h"
#include <stdint.h>
#define BUF_CAP 512

static char buf[BUF_CAP];

char *user = "UNKNOWN";
char *host = "localhost";
char dir[256] = "/";

void print_uint32(uint32_t n)
{
  uint32_t curr = n;
  int cnt = 0;
  char out[11];

  if (!n)
  {
    write_char('0');
    return;
  }

  while (curr > 0)
  {
    out[cnt] = '0' + curr % 10;
    curr /= 10;
    cnt++;
  }

  for(int i = cnt - 1; i >= 0; i--)
  {
    write_char(out[i]);
  }
}

void read_line(char *buf)
{
  int i = 0;
  uint32_t comp;
  unsigned char c;
  for (;;){
    comp = read_char();
    c = (unsigned char) comp;
    if (comp == SENTINEL){
      yield();
      continue;
    }

    if (c == '\n'){
      buf[i] = 0;
      write_char(c);
      return;
    }

    if(c == '\b') {
      if(i > 0){
        i--;
        write_string("\b \b");
      }
      continue;
    }

    if (i < BUF_CAP - 1){
      buf[i++] = c;
      write_char(c);
    }
  }
}

void split(char *buf, char **args)
{
  for (int i = 0; i >= 0; i++)
  {
    if (buf[i] == ' ') {
      buf[i] = 0;
      *args = buf + i + 1;
      return;
    }

    if (buf[i] == '\0'){
      *args = buf + i;
      return;
    }
  }
}

int main(void)
{
  char *args;

  for (;;)
  {
    write_string_color(user, GREEN);
    write_char_color('@', GREEN);
    write_string_color(host, GREEN);
    write_char(' ');
    write_string_color(dir, LIGHT_BLUE);
    write_string_color(" $ ", LIGHT_BLUE);

    read_line(buf);

    if(buf[0] == 0)
      continue;

    split(buf, &args);

    int found = 0;

    for (int i = 0; i < app_cnt; i++) {
      if (streq(apps[i].name, buf)){
        apps[i].fn(args);
        found = 1;
        break;
      }
    }

    if (!found){
      write_string("command \"");
      write_string(buf);
      write_string("\" not found\n");
    }

    continue;
  }

  return 0;
}
