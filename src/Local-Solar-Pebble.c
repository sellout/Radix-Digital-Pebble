#include "Local-Solar-Pebble.h"

#include "http.h"

int utc_offset = 0;

float longitude;
float longitude_threshold = 1;

int lst_offset = 0;

static void
have_time(int32_t new_utc_offset,
          bool is_dst,
          uint32_t unixtime,
          char const *tz_name,
          void *context) {
    if (utc_offset != new_utc_offset) {
        utc_offset = new_utc_offset;
    }
}

int calculate_solar_offset(int longitude) {
    return (longitude + 180) * DAYS / 360 - utc_offset;
}

static void
have_location(float new_latitude,
              float new_longitude,
              float altitude,
              float accuracy,
              void* context) {
    if (abs(longitude - new_longitude) > longitude_threshold) {
        longitude = new_longitude;
        lst_offset = calculate_solar_offset(longitude);
    }
}

void init_LSP(int32_t id) {
    http_set_app_id(id);
    http_register_callbacks((HTTPCallbacks){
        .time = have_time,
        .location = have_location
    }, NULL);
}

void update_LSP(void) {
    http_location_request();
    http_time_request();
}

int current_utc_offset(void) { return utc_offset; }
int current_lst_offset(void) { return lst_offset; }
