#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "string.h"

#include "Local-Solar-Pebble.h"
#include "options.h"

#define MY_UUID { 0xB3, 0x10, 0x92, 0xE1, 0x1E, 0x84, 0x4F, 0x76, 0x8F, 0x59, 0x48, 0xA8, 0xDD, 0x66, 0x77, 0x7F }
PBL_APP_INFO(HTTP_UUID,
             "Radix Digital", "Technomadic",
             0, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define NUM_COLUMNS 4

Window window;
TextLayer blank_layer;

struct clock {
    TextLayer year_layer[NUM_COLUMNS];
    TextLayer day_layer[NUM_COLUMNS];
    TextLayer subday_layer[NUM_COLUMNS];
    char year_str[NUM_COLUMNS][2];
    char day_str[NUM_COLUMNS][2];
    char subday_str[NUM_COLUMNS][2];
    int offset_seconds;
};

struct clock local_clock;
struct clock utc_clock;

char radix_str[5];

PblTm *now;

// keep these around so we can avoid redrawing when possible
int prev_year;
int prev_day;
int prev_subday;

char digit_to_radix_char(unsigned int base, int digit) {
    return (char)(digit < 10 ? digit + '0' : (digit - 10) + 'a');
}

int int_to_base_string(unsigned int base, int x, char str[][2], TextLayer *layer, int last_index, bool pad) {
    for (; x > 0; x /= base, --last_index) {
        str[last_index][0] = digit_to_radix_char(base, x % base);
        str[last_index][1] = '\0';
        text_layer_set_text(&layer[last_index], str[last_index]);
    }

    for (; last_index >= 0; --last_index) {
        text_layer_set_text(&layer[last_index], pad ? "0" : "");
    }

    return 0;
}

void draw_year(struct clock *me) {
    int year = now->tm_year + 1900;
    int_to_base_string(year_base, year, me->year_str, me->year_layer, 3, false);
}

void draw_day(struct clock *me, int offset_day) {
    int day = now->tm_yday + offset_day;
    int_to_base_string(day_base, day, me->day_str, me->day_layer, 2, false);
}

unsigned int ticks_in_day;

int draw_subday(struct clock *me) {
    int day_adjustment = 0;
    int seconds_into_day = (now->tm_hour * 60 + now->tm_min) * 60 + now->tm_sec + me->offset_seconds;
    if (seconds_into_day < 0) {
        seconds_into_day += seconds_in_day;
        day_adjustment = -1;
    } else if (seconds_in_day <= seconds_into_day) {
        seconds_into_day -= seconds_in_day;
        day_adjustment = 1;
    }
    unsigned int subday = (seconds_into_day * ticks_in_day) / seconds_in_day;
    int_to_base_string(subday_base, subday, me->subday_str, me->subday_layer, 3, true);

    return day_adjustment;
}

void init_time_layer(TextLayer *layer, GRect frame, uint32_t font_resource, bool dark) {
    text_layer_init(layer, frame);
    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_font(layer, fonts_load_custom_font(resource_get_handle(font_resource)));
    text_layer_set_text_alignment(layer, GTextAlignmentCenter);
    text_layer_set_text_color(layer, dark ? GColorWhite : GColorBlack);
    layer_add_child (&window.layer, &layer->layer);
}

static void update_clock (void) {
    draw_subday(&local_clock);
    draw_day(&local_clock, 0);
    draw_year(&local_clock);

    if (show_utc) {
        int day_offset = draw_subday(&utc_clock);
        draw_day(&utc_clock, day_offset);
    }
}


void handle_init(AppContextRef ctx) {
    window_init(&window, "Radix Digital");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorBlack);

    resource_init_current_app(&RADIX_RESOURCES);
    GRect r = layer_get_bounds(&window.layer);
    int section_height = r.size.h / 3;

    text_layer_init(&blank_layer, GRect(0, 0, r.size.w, section_height));
    text_layer_set_background_color(&blank_layer, GColorWhite);
    layer_add_child (&window.layer, &blank_layer.layer);

    int text_offset = 8;
    int zero_width = 30;

    for (size_t i = 0; i < NUM_COLUMNS; ++i) {
        init_time_layer(&local_clock.year_layer[i],
                        GRect(r.size.w - zero_width * (4 - i), - text_offset, zero_width, section_height + text_offset),
                        RESOURCE_ID_51,
                        false);
    }
    for (size_t i = 0; i < NUM_COLUMNS; ++i) {
        init_time_layer(&local_clock.day_layer[i],
                        GRect(r.size.w - zero_width * (4 - i), section_height - text_offset, zero_width, section_height + text_offset),
                        RESOURCE_ID_51,
                        true);
    }
    for (size_t i = 0; i < NUM_COLUMNS; ++i) {
        init_time_layer(&local_clock.subday_layer[i],
                        GRect(r.size.w - zero_width * (4 - i), section_height * 2 - text_offset, zero_width, section_height + text_offset),
                        RESOURCE_ID_51,
                        true);
    }

    switch (radix_point_style) {
    case DOT:
        snprintf(radix_str, 5, ".");
        break;
    case MAX_DIGIT:
        text_layer_set_font(&local_clock.day_layer[3], fonts_load_custom_font(resource_get_handle(RESOURCE_ID_13)));
        snprintf(radix_str, 5, "\n\n\n%c",
                 day_base == subday_base
                 ? digit_to_radix_char(day_base, day_base - 1)
                 : '?');
        break;
    }
    text_layer_set_text(&local_clock.day_layer[3], radix_str);

    if (show_utc) {
        int utc_section_height = section_height  / NUM_COLUMNS;
        int utc_text_offset = text_offset / NUM_COLUMNS;
        int utc_zero_width = r.size.w - zero_width * NUM_COLUMNS;

        for (size_t i = 0; i < NUM_COLUMNS; ++i) {
            init_time_layer(&utc_clock.day_layer[i],
                            GRect(0, section_height + utc_section_height * i - utc_text_offset,
                                  utc_zero_width, utc_section_height + utc_text_offset),
                            RESOURCE_ID_13,
                            true);
        }
        for (size_t i = 0; i < NUM_COLUMNS; ++i) {
            init_time_layer(&utc_clock.subday_layer[i],
                            GRect(0, section_height * 2 + utc_section_height * i - utc_text_offset,
                                  utc_zero_width, utc_section_height + utc_text_offset),
                            RESOURCE_ID_13,
                            true);
        }

        text_layer_set_text(&utc_clock.day_layer[3], ".");
    }

    // draw immediately, so we don’t start with a blank face.
    PblTm right_now;
    now = &right_now;
    get_time(now);
    update_clock();
}

static void handle_second_tick (AppContextRef ctx, PebbleTickEvent *t) {
    now = t->tick_time;
    update_clock();
}

void pbl_main(void *params) {
    ticks_in_day = subday_base * subday_base * subday_base * subday_base;

    PebbleAppHandlers handlers = {
        .init_handler = &handle_init,
        .tick_info = {
            .tick_handler = &handle_second_tick,
            .tick_units = SECOND_UNIT
        }
    };
    app_event_loop(params, &handlers);
}
