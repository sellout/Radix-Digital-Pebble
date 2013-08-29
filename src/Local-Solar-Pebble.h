#pragma once

#include <stdint.h>
#include "http.h"

// some useful time constants
#define SECONDS 1
#define MINUTES (60 * SECONDS)
#define HOURS   (60 * MINUTES)
#define DAYS    (24 * HOURS)

// This gives us the maximum precision for fractions of a day
#define RATIONAL_DAY INT_MAX
#define SECONDS_PER_TICK (RATIONAL_DAY / DAYS)

// call this to set up the HTTP request hooks
void init_LSP(int32_t id);

// call this in whatever handler you want to use to update the data
void update_LSP();

// returns the seconds difference between time zone time and UTC
int current_utc_offset();

// returns the seconds difference between time zone time and local solar time
int current_lst_offset();

bool is_dst();
