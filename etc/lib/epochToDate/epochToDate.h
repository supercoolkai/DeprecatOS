#ifndef EPOCH_TO_DATE_H
#define EPOCH_TO_DATE_H
#include <stdint.h>

void epoch_to_date(uint64_t epoch, uint64_t *year_out, uint64_t *month_out, uint64_t *day_out, uint64_t *hour_out, uint64_t *minute_out, uint64_t *second_out, uint64_t *millisecond_out);

#endif
