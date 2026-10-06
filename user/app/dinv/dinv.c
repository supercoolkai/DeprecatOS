#include "app/dinv/dinv.h"
#include "sys/syscall.h"
#include "userland/syscall/syscallController.h"
#include "app/depsh/depshCommon.h"
#include "app/depsh/cmds/touch.h"
#include "errors.h"

#include <stdint.h>
#include <stdbool.h>

#define KEY_ESC '\x1b'
#define TEXT_CAP 262144
#define COL_OFFSET 6

// D INV
// I S
// N OT
// V I

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

static uint32_t top_row;
static uint32_t left_col;

static uint32_t mode;
static bool running;

static char text[TEXT_CAP];
static uint32_t gap_start;
static uint32_t gap_end;

static char cmd_buf[64];
static uint32_t cmd_len;

static char path[256];

static uint32_t replacement_status;

static char *args_save;

static char linebuf[256];

static char complaint[64];
static bool complaining;
static uint32_t complaint_expiry;

static void handle_normal(unsigned char c);
static void handle_insert(unsigned char c);
static void handle_command(unsigned char c);

static uint32_t buf_rows(void)
{
  uint32_t r = 1;
  for (uint32_t i = 0; i < gap_start; i++) 
    if (text[i] == '\n') r++;
  for (uint32_t i = gap_end; i < TEXT_CAP; i++)
    if (text[i] == '\n') r++;

  return r;
}

static char sanitize(char c)
{
  unsigned char u = (unsigned char) c;
  if (u < 0x20 || u == 0x7F)
    return '.';

  return c;
}

static uint32_t text_cols(void)
{
  uint32_t w = sizeof(linebuf);
  if (w > screen_cols - COL_OFFSET)
    w = screen_cols - COL_OFFSET;
  return w;
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
  set_scrollback(true);
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
    char *complaint_temp = "Failed to save";
    for (uint32_t i = 0; i < 15; i++) {
      complaint[i] = complaint_temp[i];
    }
    complaining = true;
    complaint_expiry = get_ticks() + 5000;
    return;
  }

  if (fsave(ino, (uint16_t *) text, gap_start) == SYSCALL_ERROR){
    char *complaint_temp = "Failed to save";
    for (uint32_t i = 0; i < 15; i++) {
      complaint[i] = complaint_temp[i];
    }
    complaining = true;
    complaint_expiry = get_ticks() + 5000;
    return;
  }
}

static void save_quit(void)
{
  save();
  if (!complaining)
    quit();
}

static DinvCommand cmds[] = {
  {"q", quit},
  {"w", save},
  {"wq", save_quit},
  {"qa", quit},
  {"wa", save},
  {"wqa", save_quit},
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

static void check_scroll(void)
{
  uint32_t cursor_row;
  uint32_t cursor_col;
  cursor_rowcol(&cursor_row, &cursor_col);

  uint32_t width = text_cols();
  if (cursor_row < top_row){
    top_row = cursor_row;
    replacement_status = DMG_FULL;
  }
  else if (cursor_row >= top_row + text_rows){
    top_row = cursor_row - text_rows + 1;
    replacement_status = DMG_FULL;
  }

  if (cursor_col < left_col){
    left_col = cursor_col;
    replacement_status = DMG_FULL;
  }
  else if (cursor_col >= left_col + width) {
    left_col = cursor_col - width + 1;
    replacement_status = DMG_FULL;
  }
}

static void write_line_header(uint32_t row)
{
  char line_header[COL_OFFSET];
  int h_off = COL_OFFSET-3;
  uint32_t row_temp = row;
  do {
    line_header[h_off--] = (char)('0' + row_temp % 10);
    row_temp /= 10;
  } while (row_temp > 0 && h_off >= 0);

  while (h_off >= 0) {
    line_header[h_off--] = ' ';
  }

  line_header[COL_OFFSET - 2] = '>';
  line_header[COL_OFFSET - 1] = ' ';

  write_string_len_no_upd(line_header, COL_OFFSET);
}

static void gap_render(void)
{
  uint32_t cursor_row;
  uint32_t cursor_col;
  
  cursor_rowcol(&cursor_row, &cursor_col);

  uint32_t num_rows = buf_rows();

  uint32_t width = text_cols();
  switch(replacement_status){
    case(DMG_NONE):
      break;
    case(DMG_TAIL):
      {
        if (cursor_row - top_row >= text_rows)
          break;
        
        set_cursor_no_upd(cursor_row - top_row, 0);

        if (cursor_row < num_rows)
          write_line_header(cursor_row);
        

        uint32_t begin = gap_start;
        while(begin > 0 && text[begin - 1] != '\n')
          begin--;
        
        uint32_t end = gap_end;
        while(end < TEXT_CAP && text[end] != '\n')
          end++;
        
        uint32_t c = 0;
        uint32_t n = 0;
        for (uint32_t i = begin; i < gap_start && n < width; i++)
          if (c++ >= left_col) 
            linebuf[n++] = sanitize(text[i]);
        for (uint32_t i = gap_end; i < end && n < width; i++)
          if (c++ >= left_col)
            linebuf[n++] = sanitize(text[i]);
        while (n < width)
          linebuf[n++] = ' ';

        write_string_len_no_upd(linebuf, width);
      }
      break;

    case(DMG_BELOW):
      {
        if (cursor_row - top_row >= text_rows)
          break;

        uint32_t begin = gap_start;
        while(begin > 0 && text[begin - 1] != '\n')
          begin--;
        
        uint32_t end = gap_end;
        while(end < TEXT_CAP && text[end] != '\n')
          end++;

        if (cursor_row - top_row > 0) {
          uint32_t start = cursor_row - 1;
          uint32_t prev_begin = begin - 1;
          while (prev_begin > 0 && text[prev_begin - 1] != '\n')
            prev_begin--;

          uint32_t n = 0;
          uint32_t c = 0;
          for (uint32_t i = prev_begin; i < begin - 1 && n < width; i++)
            if (c++ >= left_col)
              linebuf[n++] = sanitize(text[i]);
          while (n < width)
            linebuf[n++] = ' ';

          set_cursor_no_upd(start - top_row, 0);

          if (start < num_rows)
            write_line_header(start);

          write_string_len_no_upd(linebuf, width);
        }
        
        uint32_t c = 0;
        uint32_t n = 0;
        for (uint32_t i = begin; i < gap_start && n < width; i++)
          if (c++ >= left_col) 
            linebuf[n++] = sanitize(text[i]);
        for (uint32_t i = gap_end; i < end && n < width; i++)
          if (c++ >= left_col)
            linebuf[n++] = sanitize(text[i]);
        while (n < width)
          linebuf[n++] = ' ';
        
        set_cursor_no_upd(cursor_row - top_row, 0);

        if (cursor_row < num_rows)
          write_line_header(cursor_row);

        write_string_len_no_upd(linebuf, width);

        uint32_t next_begin = (end < TEXT_CAP) ? end + 1 : TEXT_CAP;
        uint32_t next_end;
        for (uint32_t r = cursor_row + 1; r - top_row < text_rows; r++) {
          next_end = next_begin;
          while(next_end < TEXT_CAP && text[next_end] != '\n')
            next_end++;
          
          n = 0;
          c = 0;
          for (uint32_t i = next_begin; i < next_end && n < width; i++)
            if (c++ >= left_col)
              linebuf[n++] = sanitize(text[i]);
          while (n < width)
            linebuf[n++] = ' ';

          set_cursor_no_upd(r - top_row, 0);
          
          if (r < num_rows)
            write_line_header(r);
          else
            write_string_len_no_upd("      ", COL_OFFSET);

          write_string_len_no_upd(linebuf, width);
          

          next_begin = (next_end < TEXT_CAP) ? next_end + 1 : TEXT_CAP;
        }
      }
      break;

    case (DMG_FULL):
      {
        uint32_t pos = 0;
        if (pos == gap_start)
          pos = gap_end;

        for (uint32_t skip = 0; skip < top_row; skip++) {
          while (pos < TEXT_CAP && text[pos] != '\n'){
            pos++;
            if (pos == gap_start)
              pos = gap_end;
          }
          if (pos < TEXT_CAP) {
            pos++;
            if (pos == gap_start){
              pos = gap_end;
            }
          }
        }

        for (uint32_t r = 0; r < text_rows; r++){
          uint32_t n = 0;
          uint32_t c = 0;
          while (pos < TEXT_CAP && text[pos] != '\n') {
            if (c++ >= left_col && n < width)
              linebuf[n++] = sanitize(text[pos]);
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

          if (r + top_row < num_rows)
            write_line_header(r + top_row);
          else
            write_string_len_no_upd("      ", COL_OFFSET);

          write_string_len_no_upd(linebuf, width);
        }
      }
      break;
    case (DMG_CMD):
      {
        uint32_t width_to_use = width + COL_OFFSET;
        if (width_to_use> sizeof(linebuf)) {
          width_to_use = sizeof(linebuf);
        }
        uint32_t cmd_col = 1;
        set_cursor_no_upd(screen_rows-1, cmd_col);
        
        uint32_t n = 0;
        while(n < width_to_use-2)
          linebuf[n++] = ' ';

        write_string_len_no_upd(linebuf, width_to_use-2);

        set_cursor_no_upd(screen_rows-1, cmd_col);
        write_string_len_no_upd(cmd_buf, cmd_len);
      }
      break;
  }
  
  set_cursor_no_upd(screen_rows - 1, 0);
  if (!complaining){
    switch(mode) {
      case(MODE_NORMAL):
        write_string_no_upd("NORMAL");
        break;
      case(MODE_INSERT):
        write_string_no_upd("INSERT");
        break;

      case (MODE_COMMAND):
        write_char_no_upd(':');
        break;
    }
  }
  else{
    write_string_no_upd(complaint);
  }

  
  if (mode == MODE_COMMAND){
    if (cmd_len >= screen_cols - 1)
      set_cursor(screen_rows-1, screen_cols - 1);
    else{
      set_cursor(screen_rows-1, cmd_len + 1);
    }
  }
  else{
    uint32_t row;
    uint32_t col;
    cursor_rowcol(&row, &col);
    set_cursor(row - top_row, COL_OFFSET + col - left_col);
  }

}

static void handle_command(unsigned char c)
{
  mode = MODE_COMMAND;

  if (c == SENTINEL)
    return;

  if (c == KEY_ESC){
    mode = MODE_NORMAL;
    replacement_status = DMG_CMD;
    cmd_len = 0;
    return;
  }

  if (c == '\b'){
    if (cmd_len > 0){
      cmd_len --;
      replacement_status = DMG_CMD;
    }
    return;
  }

  if (c == '\n') {
    cmd_buf[cmd_len] = '\0';
    bool found= false;
    for (uint32_t i = 0; i < cmd_cnt; i++) {
      if (streq(cmds[i].name, cmd_buf)) {
        found = true;;
        cmds[i].fn();
      }
    }

    if (!found) {
      complaint_expiry = get_ticks() + 5000;

      char *complaint_temp = "Command not found";
      for (uint32_t i = 0; i < 18; i++) {
        complaint[i] = complaint_temp[i];
      }

      complaining = true;
    }

    mode = MODE_NORMAL;
    replacement_status = DMG_CMD;
    cmd_len = 0;
    return;
  }

  if (cmd_len < sizeof(cmd_buf) - 1){
    cmd_buf[cmd_len++] = c;
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
      gap_start --;
    }
    return;
  }
  if (gap_start == gap_end) return;

  replacement_status = (c == '\n') ? DMG_BELOW : DMG_TAIL;

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
    complaining = false;
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

  if (screen_cols <= COL_OFFSET) {
    write_string("dinv: resolution is too small to run\n");
    return;
  }

  // bottom row reserved for cmd line
  text_rows = screen_rows - 1;

  running = true;
  mode = MODE_NORMAL;
  gap_start = 0;
  gap_end = TEXT_CAP;
  cmd_len = 0;
  complaining = false;
  replacement_status = DMG_FULL;
  top_row = 0;
  left_col = 0;

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

  uint32_t path_n = resolve_dir(path);
  if (path_n != SYSCALL_ERROR){
    if (is_dir(path_n)){
      write_string("dinv: dir provided, must give a file\n");
      return;
    }
    
    get_stat(path_n, (uint32_t *)stat_buf);
    if (*(uint32_t*)(stat_buf + 4) >= TEXT_CAP){
      write_string("dinv: file is too large to render properly\n");
      return;
    }

    uint32_t n = 0;
    uint32_t r;
    for (;;) {
      r = read_chunk(path_n, n, fs_buf);
      if (r == 0){
        break;
      }

      if (r == SYSCALL_ERROR) {
        write_string("dinv: an unknown error occurred while reading file ");
        write_string(args);
        write_string("\n");

        return;
      }

      else {
        char *blk_bytes = (char *) fs_buf;

        for (uint32_t i = 0; i < r; i++) {
          text[gap_start++] = blk_bytes[i];
        }

        n++;
      }
    }
  }
  check_scroll();
  gap_render();

  set_scrollback(false);
  while (running){ 
    uint32_t comp = read_char();
    if (comp == SENTINEL){
      if (complaining && get_ticks() >= complaint_expiry) {
        complaining = false;
        replacement_status = DMG_CMD;
        gap_render();
      }
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
    check_scroll();
    gap_render();
  }

  clear_screen();
  return;
}
