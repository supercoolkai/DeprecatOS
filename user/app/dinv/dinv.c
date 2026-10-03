#include "app/dinv/dinv.h"
#include "sys/syscall.h"
#include "userland/syscall/syscallController.h"
#include "app/depsh/depshCommon.h"
#include "app/depsh/cmds/touch.h"
#include "errors.h"

#include <stdint.h>
#include <stdbool.h>

#define KEY_ESC '\x1b'
#define TEXT_CAP 16384
#define GROW_AMT 32

enum {
  MODE_NORMAL,
  MODE_INSERT,
  MODE_COMMAND
};

typedef struct {
  char *name; 
  void (*fn)(void);
} DinvCommand;

enum {
  DMG_NONE,
  DMG_TAIL,
  DMG_CMD,
  DMG_BELOW,
  DMG_FULL
};

// NOTE: not one-read statics cuz if you adjust the window size
// (especially while running it in an emulator)
// it turns into the old resolution even tho
// it updated. worse when i add a wm

static uint32_t screen_rows;
static uint32_t screen_cols;
static uint32_t text_rows;

static uint32_t curr_col;
static uint32_t curr_row;

static uint32_t mode;
static bool running;

static char text[TEXT_CAP];
static uint32_t gap_start;
static uint32_t gap_end;

static char cmd_buf[64];
static uint32_t cmd_len;

static char path[256];
static const char *parent_path;

static uint32_t replacement_status;

static char *args_save;

static char linebuf[256];

static void handle_normal(unsigned char c);
static void handle_insert(unsigned char c);
static void handle_command(unsigned char c);

static void strcat(char *dst, const char *src)
{
  uint32_t i = 0;
  while (dst[i] != '\0')
    i++;

  uint32_t j = 0;
  while (src[j] != '\0') {
    dst[i] = src[j];
    i++;
    j++;
  }
  dst[i] = '\0';
}

static void cursor_right(void)
{
  if (gap_end == TEXT_CAP)
    return;

  text[gap_start] = text[gap_end];
  gap_start++;
  gap_end++;
}

static void cursor_left(void)
{
  if (gap_start == 0)
    return;

  gap_end--;
  gap_start--;
  text[gap_end] = text[gap_start];
}

static void quit(void)
{
  running = false;
}

static void save(void)
{
  while (gap_end != TEXT_CAP)
    cursor_right();

  if (resolve_dir(path) == SYSCALL_ERROR){
    cmd_touch(args_save);
  }

  uint32_t ino = resolve_dir(path);
  if (ino == SYSCALL_ERROR){
    running = false;
    write_string("dinv: failed to save path!! crashing without progress saved..\n");
    return;
  }

  fsave(ino, (uint16_t *) text, gap_start);
}

static void save_quit(void)
{
  save();
  quit();
}

static DinvCommand cmds[] = {
  {"q", quit},
  {"w", save},
  {"wq", save_quit},
};
static uint32_t cmd_cnt = sizeof(cmds) / sizeof(cmds[0]);

static void cursor_rowcol(uint32_t *row, uint32_t *col)
{
  uint32_t r = 0;
  uint32_t c = 0;
  for (uint32_t i = 0; i < gap_start; i++){
    if (text[i] == '\n') {
      r++;
      c = 0;
    }
    else{
      c++;
    }
  }

  *col = c;
  *row = r;
}

static void gap_render(void)
{
  uint32_t cursor_row;
  uint32_t cursor_col;
  
  cursor_rowcol(&cursor_row, &cursor_col);

  cursor_col = 0;

  uint32_t width;
  if (screen_cols > sizeof(linebuf))
    width = sizeof(linebuf);
  else
    width = screen_cols;

  switch(replacement_status){
    case(DMG_NONE):
      break;
    case(DMG_TAIL):
      {
        if (cursor_row >= text_rows)
          break;
        set_cursor_no_upd(cursor_row, cursor_col);
        
        uint32_t begin = gap_start;
        while(begin > 0 && text[begin - 1] != '\n')
          begin--;
        
        uint32_t end = gap_end;
        while(end < TEXT_CAP && text[end] != '\n')
          end++;
        
        uint32_t n = 0;
        for (uint32_t i = begin; i < gap_start && n < width; i++)
          linebuf[n++] = text[i];
        for (uint32_t i = gap_end; i < end && n < width; i++)
          linebuf[n++] = text[i];
        while (n < width)
          linebuf[n++] = ' ';

        write_string_len_no_upd(linebuf, width);
      }
      break;

    case(DMG_BELOW):
      {
        if (cursor_row >= text_rows)
          break;

        uint32_t begin = gap_start;
        while(begin > 0 && text[begin - 1] != '\n')
          begin--;
        
        uint32_t end = gap_end;
        while(end < TEXT_CAP && text[end] != '\n')
          end++;

        uint32_t start = (cursor_row > 0) ? cursor_row - 1 : 0;

        if (cursor_row > 0) {
          uint32_t prev_begin = begin - 1;
          while (prev_begin > 0 && text[prev_begin - 1] != '\n')
            prev_begin--;

          uint32_t n = 0;
          for (uint32_t i = prev_begin; i < begin - 1 && n < width; i++)
            linebuf[n++] = text[i];
          while (n < width)
            linebuf[n++] = ' ';

          set_cursor_no_upd(start, cursor_col);
          write_string_len_no_upd(linebuf, width);
        }
        
        uint32_t n = 0;
        for (uint32_t i = begin; i < gap_start && n < width; i++)
          linebuf[n++] = text[i];
        for (uint32_t i = gap_end; i < end && n < width; i++)
          linebuf[n++] = text[i];
        while (n < width)
          linebuf[n++] = ' ';
        
        set_cursor_no_upd(cursor_row, cursor_col);

        write_string_len_no_upd(linebuf, width);

        uint32_t next_begin = (end < TEXT_CAP) ? end + 1 : TEXT_CAP;
        uint32_t next_end;
        for (uint32_t r = cursor_row + 1; r < text_rows; r++) {
          next_end = next_begin;
          while(next_end < TEXT_CAP && text[next_end] != '\n')
            next_end++;
          
          n = 0;
          for (uint32_t i = next_begin; i < next_end && n < width; i++) {
            linebuf[n++] = text[i];
          }
          while (n < width)
            linebuf[n++] = ' ';

          set_cursor_no_upd(r, cursor_col);
          
          write_string_len_no_upd(linebuf, width);
          

          next_begin = (next_end < TEXT_CAP) ? next_end + 1 : TEXT_CAP;
        }
      }
      break;

    case (DMG_FULL):
      {
        clear_screen();
        uint32_t pos = 0;
        if (pos == gap_start)
          pos = gap_end;

        for (uint32_t r = 0; r < text_rows; r++){
          uint32_t n = 0;
          while (pos < TEXT_CAP && text[pos] != '\n') {
            if (n < width)
              linebuf[n++] = text[pos];
            pos++;

            if (pos == gap_start)
              pos = gap_end;
          }

          if (pos < TEXT_CAP){
            pos++;
            if (pos == gap_start)
              pos = gap_end;
          }

          while (n < width)
            linebuf[n++] = ' ';

          set_cursor_no_upd(r, 0);
          write_string_len_no_upd(linebuf, width);
        }
      }
      break;
    case (DMG_CMD):
      {
        uint32_t cmd_col = 1;
        set_cursor_no_upd(screen_rows-1, cmd_col);
        
        uint32_t n = 0;
        while(n < width-2)
          linebuf[n++] = ' ';

        write_string_len_no_upd(linebuf, width-2);

        set_cursor_no_upd(screen_rows-1, cmd_col);
        write_string_len_no_upd(cmd_buf, cmd_len);
      }
      break;
  }
  
  set_cursor_no_upd(screen_rows - 1, 0);
  
  switch(mode) {
    case(MODE_NORMAL):
      write_string("NORMAL");
      break;
    case(MODE_INSERT):
      write_string("INSERT");
      break;

    case (MODE_COMMAND):
      write_char(':');
      break;
  }

  
  uint32_t row;
  uint32_t col;
  cursor_rowcol(&row, &col);
  set_cursor(row, col);

  if (mode == MODE_COMMAND)
    set_cursor(screen_rows-1, cmd_len + 1);
}

static void handle_command(unsigned char c)
{
  mode = MODE_COMMAND;

  if (c == SENTINEL)
    return;

  if (c == KEY_ESC){
    mode = MODE_NORMAL;
    return;
  }

  if (c == '\b'){
    if (cmd_len > 0){
      uint32_t row;
      uint32_t col;
      cursor_rowcol(&row, &col);

      cmd_len --;
      replacement_status = DMG_CMD;
    }
    return;
  }

  if (c == '\n') {
    cmd_buf[cmd_len] = '\0';
    for (uint32_t i = 0; i < cmd_cnt; i++) {
      if (streq(cmds[i].name, cmd_buf)) {
        cmds[i].fn();
      }
    }
    mode = MODE_NORMAL;
    return;
  }

  if (cmd_len < sizeof(cmd_buf) - 1){
    cmd_buf[cmd_len++] = c;
    uint32_t row;
    uint32_t col;
    cursor_rowcol(&row, &col);
    replacement_status = DMG_CMD;
  }
}

static void handle_insert(unsigned char c)
{
  mode = MODE_INSERT;
  if (c == SENTINEL){
    return;
  }

  if (c == KEY_ESC) {
    mode = MODE_NORMAL;
    return;
  }
  if (c == '\b'){
    if (gap_start > 0){
      replacement_status = (text[gap_start - 1] == '\n') ? DMG_BELOW : DMG_TAIL;
      uint32_t row;
      uint32_t col;
      cursor_rowcol(&row, &col);
      gap_start --;
    }
    return;
  }
  if (gap_start == gap_end) return;

  replacement_status = (c == '\n') ? DMG_BELOW : DMG_TAIL;
  uint32_t row;
  uint32_t col;
  cursor_rowcol(&row, &col);

  text[gap_start] = c;
  gap_start++;
}

static void handle_normal(unsigned char c)
{
  mode = MODE_NORMAL;
  if (c == SENTINEL) 
    return;

  if (c == ':'){
    cmd_len = 0;
    mode = MODE_COMMAND;
    replacement_status = DMG_CMD;
    return;
  }

  if (c == 'i'){
    mode = MODE_INSERT;
    return;
  }

  if (c == 'a'){
    cursor_right();
    mode = MODE_INSERT;
    return;
  }

  if (c == 'h') {
    cursor_left();
    return;
  }

  if (c == 'j'){ 
    uint32_t p = gap_end;

    uint32_t col;
    uint32_t row;

    cursor_rowcol(&row, &col);

    while (p < TEXT_CAP && text[p] != '\n')
      p++;

    if (p == TEXT_CAP)
      return;

    while(gap_end != p + 1)
      cursor_right();

    for (uint32_t k = 0; k < col; k++){
      if (gap_end == TEXT_CAP || text[gap_end] == '\n')
        break;

      cursor_right();
    }



    return;
  }

  if (c == 'k') {
    uint32_t p = gap_start;

    uint32_t col;
    uint32_t row;

    cursor_rowcol(&row, &col);

    while (p > 0 && text[p - 1] != '\n')
      p--;

    if (p == 0)
      return;
    
    while(gap_start != p - 1)
      cursor_left();

    for (uint32_t k = 0; k < col; k++) {
      if (gap_start == 0 || text[gap_start] == '\n'){
        break;
      }

      cursor_left();
    }
    uint32_t col_temp;
    cursor_rowcol(&row, &col_temp);
    while(col_temp > col){
      cursor_left();
      col_temp--;
    }

    return;
  }

  if (c == 'l') {
    cursor_right();
    return;
  }

  return;
}

void app_dinv(char *args)
{
  if (args[0] == '\0'){
    write_string("dinv: requires file path\n");
    return;
  }
  clear_screen();
  get_screen_dims(&screen_rows, &screen_cols);

  // bottom row reserved for cmd line
  text_rows = screen_rows - 1;

  running = true;
  mode = MODE_NORMAL;
  gap_start = 0;
  gap_end = TEXT_CAP;
  cmd_len = 0;

  parent_path = return_path(".");
  if (!parent_path)
    return;

  const char *full = return_path(args);
  if (!full){
    write_string("dinv: current path is too long to fully parse\n");
    return;
  }

  uint32_t n = 0;
  while (n < sizeof(path) - 1 && full[n] != '\0'){
    path[n] = full[n];
    n++;
  }
  path[n] = '\0';

  args_save = args;



  gap_render();
  while (running){ 
    uint32_t comp = read_char();
    if (comp == SENTINEL){
      yield();
      continue;
    }


    unsigned char c = (unsigned char) comp;
    
    replacement_status = DMG_NONE;

    switch (mode) {
      case MODE_NORMAL:
        handle_normal(c);
        break;
      
      case MODE_INSERT:
        handle_insert(c);
        break;
      
      case MODE_COMMAND:
        handle_command(c);
        break;
    }
    gap_render();
  }

  clear_screen();
  return;
}
