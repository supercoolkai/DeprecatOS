#include "sys/syscall.h"
#include "drivers/fb/fbController.h"
#include "userland/syscall/syscallController.h"
#include "app/appCtl.h"
#include "streq/streq.h"
#include "keys/ctrlkeys.h"
#include <stdint.h>
#define BUF_CAP 512

static char buf[BUF_CAP];

char *user = "UNKNOWN";
char *host = "localhost";
char dir[256] = "/";

static uint32_t max_len;

static uint32_t anchor;

static uint32_t screen_cols;
static uint32_t screen_rows;

static uint32_t cur;


static void place_cursor(void)
{
  set_cursor((anchor + cur) / screen_cols, (anchor + cur) % screen_cols);
}

static void resync(bool is_insert)
{
  uint32_t r;
  uint32_t c;
  get_cursor(&r, &c);

  anchor = r * screen_cols + c - ((is_insert) ? max_len : max_len+1);

  place_cursor();
}

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
  cur = 0;
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
      buf[max_len] = 0;
      cur = max_len;
      place_cursor();
      write_char('\n');
      return;
    }

    if(c == '\b') {
      if(cur > 0){
        for (uint32_t i = cur - 1; i < max_len - 1; i ++)
          buf[i] = buf[i+1];
        max_len--;
        cur--;
        place_cursor();
        write_string_len(&buf[cur], max_len - cur);
        write_char(' ');
        resync(false);
      }
      continue;
    }

    if (c == CTRL_KEY_LEFT) {
      if(cur > 0) {
        cur--;
        place_cursor();
      }
      continue;
    }

    if (c == CTRL_KEY_RIGHT){
      if (cur < max_len) {
        cur++;
        place_cursor();
      }
      continue;
    }

    if (max_len < BUF_CAP - 1){
      for (uint32_t i = max_len; i > cur; i --)
        buf[i] = buf[i-1];
      buf[cur] = c;
      max_len++;
      write_string_len(&buf[cur], max_len - cur);
      cur++;
      resync(true);
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

  get_screen_dims(&screen_rows, &screen_cols);

  uint32_t prompt_row;
  uint32_t prompt_col;

  for (;;)
  {
    write_string_color(user, GREEN);
    write_char_color('@', GREEN);
    write_string_color(host, GREEN);
    write_char(' ');
    write_string_color(dir, LIGHT_BLUE);
    write_string_color(" $ ", LIGHT_BLUE);
  
    get_cursor(&prompt_row, &prompt_col);
    anchor = prompt_row * screen_cols + prompt_col;
    max_len = 0;
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
