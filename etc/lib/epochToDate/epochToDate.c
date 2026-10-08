#include "epochToDate/epochToDate.h"
#include "longops/longops.h"
#include <stdint.h>

static uint32_t days_per_month[] = {
  31, 28, 31, 30, 31, 30,
  31, 31, 30, 31, 30, 31
};

static bool is_leap_year(uint64_t year)
{
  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
    return true;
  }
  return false;
}

void epoch_to_date(uint64_t epoch, uint64_t *year_out, uint64_t *month_out, uint64_t *day_out, uint64_t *hour_out, uint64_t *minute_out, uint64_t *second_out, uint64_t *millisecond_out)
{
  uint64_t millisecond = epoch % 1000;
  epoch /= 1000;

  uint64_t second = epoch % 60;
  epoch /= 60;

  uint64_t minute = epoch % 60;
  epoch /= 60;

  uint64_t hour = epoch % 24;
  uint64_t day_of_epoch = epoch / 24;

  bool leap = false;

  uint64_t year = 1970;
  uint32_t days_in_year;

  for (;;) {
    days_in_year = is_leap_year(year) ? 366 : 365;
    if (day_of_epoch < days_in_year)
      break;

    day_of_epoch -= days_in_year;
    year++;
  }

  if ((leap = is_leap_year(year))){
    days_per_month[1]++;
  }

  uint64_t month = 1;

  while (day_of_epoch >= days_per_month[month - 1]) {
    day_of_epoch -= days_per_month[month - 1];
    month++;
  }

  if (leap) {
    days_per_month[1]--;
  }

  *year_out = year;
  *month_out = month;
  *day_out = day_of_epoch + 1;
  *hour_out = hour;
  *minute_out = minute;
  *second_out = second;
  *millisecond_out = millisecond;
}
