#include "shell/sys/syscall.h"
#include <stdint.h>
#include <stdbool.h>
#define BUF_CAP 512
#define BLACK 0
#define BLUE 1
#define GREEN 2
#define RED 4
#define LIGHT_BLUE 9
#define YELLOW 14
#define WHITE 15
#define BIT_32_PER_BLK 1024
#define COLS_PER_ROW_LS 5
#define NO_PERMISSION_MASK 0xF000
#define INODE_DIR_TYPE 0x4000

static char buf[BUF_CAP];
static uint32_t fs_buf[BIT_32_PER_BLK];
static char *blk_bytes;
static uint8_t stat_buf[128];

char *user = "UNKNOWN";
char *host = "localhost";
static char dir[256] = "/";

static char full_path[256];

typedef struct {
  char *name;
  void (*fn)(char *args);
} Command;

int streq(char *a, char *b)
{
  int i = 0;

  while (a[i] == b[i])
  {
    if (a[i] == 0)
      return 1;

    i++;
  }

  return 0;
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

static bool upd_full_path(void)
{
  for (int i = 0; i < 256; i++) {
    full_path[i] = 0;
  }
  int i = 0;
  while(i < 256){
    char c = dir[i];
    if (c == 0)
      return true;

    full_path[i] = c;

    i++;
  }
  
  return false;
}

static void cmd_help(char *args)
{
  if (!streq(args, ""))
  {
    write_string("\nhelp: option ");
    write_string(args);
    write_string(" does not exist\n");
    return;
  }

  write_string("help:"
               " Lists all available commands"
               " with a short description of each.\n"
               "echo [arg ...]: Prints a given string.\n"
               "ticks: Prints the amount of timer ticks accumulated"
               " since boot.\n"
               "cat [path ...]: Reads the bytes of a given file.\n"
               "ls [path ...]: Lists the subdirectories/files of a directory.\n"
               "cd [path ...]: Changes the current directory into the entered path.\n"
               "stat [-t, -s, -l] [path ...]: Lists the properties of the given file/dir.\n");
}

static void cmd_echo(char *args)
{
  write_string(args);
  write_string("\n");
}

static void cmd_ticks(char *args)
{
  if (!streq(args, ""))
  {
    write_string("ticks: option ");
    write_string(args);
    write_string(" does not exist\n");
    return;
  }
  
  print_uint32(get_ticks());
  write_string("\n");
}

static const char *return_path(char *args)
{
  const char *path;

  if (args[0] != '/') {
    bool success = upd_full_path();

    if (!success){
      return 0;
    }

    int i = 0;
    while (full_path[i] != 0) 
      i++;

    full_path[i++] = '/';

    int j = 0;
    while (args[j] != 0) {
      if (i + j >= 255) {
        return 0;
      }

      full_path[i + j] = args[j];
      j++;
    }

    full_path[i + j] = 0;

    path = full_path;
  }

  else{
    path = args;
  }

  return path;
}

static void cmd_stat(char *args)
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

  if (inode_n == 0xFFFFFFFF) {
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

static bool is_dir(uint32_t inode_n)
{
  get_stat(inode_n, (uint32_t *)stat_buf);

  return ((*(uint16_t *) stat_buf & NO_PERMISSION_MASK) == INODE_DIR_TYPE);
}

static void cmd_cat(char *args)
{
  if (streq(args, "")) {
    write_string("cat: must provide a file to read");
    return;
  }

  const char *path = return_path(args);

  if (path == 0)
  {
    write_string("cat: current path is too long to fully parse\n");
    return;
  }

  uint32_t inode_n = resolve_dir(path);

  if (inode_n == 0xFFFFFFFF) {
    write_string("cat: file ");
    write_string(args);
    write_string(" does not exist or is not a file\n");
    return;
  }

  if (is_dir(inode_n)) {
    write_string("cat: cannot read a dir\n");
    return;
  }

  uint32_t n = 0;
  uint32_t r;
  for (;;) {
    r = read_chunk(inode_n, n, fs_buf);
    if (r == 0){
      write_char_color('%', GREEN);
      write_char('\n');
      return;
    }

    if (r == 0xFFFFFFFF) {
      write_string("cat: an unknown error occurred while reading file ");
      write_string(args);
      write_string("\n");

      return;
    }

    else {
      blk_bytes = (char *) fs_buf;

      write_string_len(blk_bytes, r);

      n++;
    }
  }
}

static void cmd_ls(char *args)
{
  const char *path = return_path(args); 

  if (path == 0)
  {
    write_string("ls: current path is too long to fully parse\n");
    return;
  }

  uint32_t inode_n = resolve_dir(path);

  if (inode_n == 0xFFFFFFFF) {
    write_string("ls: directory ");
    write_string(args);
    write_string(" does not exist or is not a directory\n");
    return;
  }

  if (!is_dir(inode_n)) {
    write_string("ls: cannot list a file\n");
    return;
  }

  uint32_t n = 0;
  uint32_t r;
  int writes = 0;
  for (;;) {
    r = read_chunk(inode_n, n, fs_buf);
    if (r == 0){
      write_char('\n');
      return;
    }

    if (r == 0xFFFFFFFF) {
      write_string("ls: an unknown error occurred while reading dir ");
      write_string(args);
      write_string("\n");

      return;
    }

    else {
      blk_bytes = (char *) fs_buf;

      uint32_t pos = 0;

      while (pos < r)
      {
        uint32_t inode = *(uint32_t *)(blk_bytes + pos);
        uint16_t advance_amt = *(uint16_t *)(blk_bytes + pos + 4);
        uint8_t name_len = blk_bytes[pos + 6];
        uint8_t type_or_name_len_hi = blk_bytes[pos + 7];

        uint8_t name[name_len];

        for (int i = 0; i < name_len; i++) {
          name[i] = blk_bytes[pos + 8 + i];
        }

        if (advance_amt < 8 || 8 + name_len > advance_amt || pos + advance_amt > r) {
          write_string("ls: a file in dir ");
          write_string(args);
          write_string(" was out of range\n");
          return;
        }

        if (inode != 0 && name[0] != '.') {
          write_string_len((const char *) name, name_len);

          write_char(' ');

          if (writes % COLS_PER_ROW_LS == 0 && writes > 0)
            write_char('\n');
          
          writes++;
        }
        pos += advance_amt;
      }

      n++;
    }


  }
}

static void cmd_cd(char *args)
{
  char *path = (char *) return_path(args);

  if (path == 0) {
    write_string("cd: current path is too long to fully parse\n");
    return;
  }

  if (args[0] == 0) {
    path = "/";
  }

  uint32_t d = resolve_dir(path);

  if (d == 0xFFFFFFFF)
  {
    write_string("cd: dir not found\n");
    return;
  }

  if (!is_dir(d)) {
    write_string("cd: cannot cd into a file\n");
    return;
  }

  int i = 0;
  int j = 0;
  int to_write = 0;

  while (path[i] != 0) {
    while (path[i] == '/') 
      i++;

    j = i;

    while (path[i] != '/' && path[i] != 0) 
      i++;

    int len = i - j;

    if (len == 1 && path[j] == '.')
      continue;


    if (len == 2 && path[j] == '.' && path[j+1] == '.'){
      while (to_write > 0 && dir[to_write - 1] != '/')
        to_write--;

      if (to_write > 0)
        to_write --;

      continue;
    }

    dir[to_write] = '/';
    to_write++;

    for (int k = 0; k < len; k++) {
      dir[to_write] = path[j + k];
      to_write++;
    }
  }

  if (to_write == 0){
    dir[to_write++] = '/';
  }

  dir[to_write] = 0;

}

static Command commands[] =
{
  {"help", cmd_help},
  {"echo", cmd_echo},
  {"ticks", cmd_ticks},
  {"cat", cmd_cat},
  {"ls", cmd_ls},
  {"cd", cmd_cd},
  {"stat", cmd_stat},
};

int main(void)
{
  unsigned char c;
  uint32_t comp;
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

    for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++){
      if (streq(commands[i].name, buf)) {
        commands[i].fn(args);
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
