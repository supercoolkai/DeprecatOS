#include "app/depsh/cmds/date.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"
#include "epochToDate/epochToDate.h"
#include "errors.h"

void cmd_date(char *args)
{
  if (!streq(args, ""))
  {
    write_string("date: option ");
    write_string(args);
    write_string(" does not exist\n");
    return;
  }
  
  uint64_t epoch;
  get_epoch(&epoch);
  
  uint64_t millisecond;
  uint64_t second;
  uint64_t minute;
  uint64_t hour;
  uint64_t day;
  uint64_t month;
  uint64_t year;

  epoch_to_date(epoch, &year, &month, &day, &hour, &minute, &second, &millisecond);

  print_uint64(day);
  write_char('/');
  print_uint64(month);
  write_char('/');
  print_uint64(year);
  write_char(' ');

  print_uint64_pad(hour, 2);
  write_char(':');
  print_uint64_pad(minute, 2);
  write_char(':');
  print_uint64_pad(second, 2);
  write_char('.');
  print_uint64_pad(millisecond, 3);

  write_char('\n');
}
