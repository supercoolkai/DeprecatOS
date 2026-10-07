#include "sys/syscall.h"
#include "colors/colors.h"
#include "userland/syscall/syscallController.h"
#include "app/appCtl.h"
#include "streq/streq.h"
#include "keys/ctrlkeys.h"
#include <stdint.h>

#define BUF_CAP 512
#define HISTORY_CAP 32

static char buf[BUF_CAP];
static char history[HISTORY_CAP][BUF_CAP];
static char draft[BUF_CAP];

static uint32_t history_head;
static uint32_t history_count;
static uint32_t history_pos;

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

static void replace_line(char *src)
{
  uint32_t old_len = max_len;
  uint32_t src_len = 0;

  while(src[src_len] != '\0' && src_len < BUF_CAP - 1){
    buf[src_len] = src[src_len];
    src_len++;
  }
  max_len = src_len;
  cur = max_len;
  set_cursor(anchor / screen_cols, anchor % screen_cols);

  write_string_len(buf, max_len);
  for (uint32_t i = max_len; i < old_len; i++)
    write_char(' ');

  uint32_t r;
  uint32_t c;
  get_cursor(&r, &c);

  anchor = r * screen_cols + c - ((max_len > old_len) ? max_len : old_len);
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
  history_pos = 0;
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

    if (c == CTRL_KEY_UP){
      if (history_pos < history_count) {
        if (history_pos == 0){
          for (uint32_t i = 0; i < max_len; i++) {
            draft[i] = buf[i];
          }
          draft[max_len] = '\0';
        }
        history_pos++;
        replace_line(history[(history_head + HISTORY_CAP - history_pos) % HISTORY_CAP]);
      }
      continue;
    }

    if (c == CTRL_KEY_DOWN) {
      if (history_pos > 0){
          history_pos--;

        if (history_pos == 0) {
          replace_line(draft);
        }
        else{
          replace_line(history[(history_head + HISTORY_CAP - history_pos) % HISTORY_CAP]);
        }
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
    
    if (!streq(history[(history_head + HISTORY_CAP - 1) % HISTORY_CAP], buf)){
      uint32_t i = 0;
      while(buf[i] != '\0'){
        history[history_head][i] = buf[i];
        i++;
      }
      history[history_head][i] = '\0';
      history_head = (history_head + 1) % HISTORY_CAP;
      if (history_count < HISTORY_CAP)
        history_count++;
    }


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
