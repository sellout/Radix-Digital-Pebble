#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "string.h"

#include "options.h"

#define MY_UUID { 0xB3, 0x10, 0x92, 0xE1, 0x1E, 0x84, 0x4F, 0x76, 0x8F, 0x59, 0x48, 0xA8, 0xDD, 0x66, 0x77, 0x7F }
PBL_APP_INFO(MY_UUID,
             "Radix Digital", "Technomadic",
             0, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define NUM_COLUMNS 4

Window window;
TextLayer blank_layer;
TextLayer year_layer[NUM_COLUMNS];
TextLayer day_layer[NUM_COLUMNS];
TextLayer subday_layer[NUM_COLUMNS];

char year_str[NUM_COLUMNS][2];
char day_str[NUM_COLUMNS][2];
char subday_str[NUM_COLUMNS][2];
char radix_str[5];

PblTm *now;

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

void draw_year(TextLayer *me) {
    int year = now->tm_year + 1900;
    int_to_base_string(base, year, year_str, me, 3, false);
}

void draw_day(TextLayer *me) {
    int day = now->tm_yday;
    int_to_base_string(base, day, day_str, me, 2, false);
}

unsigned int const seconds_in_day = 86400;
unsigned int ticks_in_day;

void draw_subday(TextLayer *me) {
    unsigned int seconds_into_day = ((now->tm_hour * 60 + now->tm_min) * 60 + now->tm_sec);
    unsigned int subday = (seconds_into_day * ticks_in_day) / seconds_in_day;
    int_to_base_string(base, subday, subday_str, subday_layer, 3, true);
}

void init_time_layer(TextLayer *layer, GRect frame, bool dark) {
    GFont font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_51));
    text_layer_init(layer, frame);
    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_font(layer, font);
    text_layer_set_text_alignment(layer, GTextAlignmentCenter);
    text_layer_set_text_color(layer, dark ? GColorWhite : GColorBlack);
    layer_add_child (&window.layer, &layer->layer);
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

    int text_offset = 10;
    int zero_width = 30;

    for (size_t i = 0; i < NUM_COLUMNS; ++i) {
        init_time_layer(&year_layer[i],
                        GRect(r.size.w - zero_width * (4 - i), - text_offset, zero_width, section_height + text_offset),
                        false);
    }
    for (size_t i = 0; i < NUM_COLUMNS; ++i) {
        init_time_layer(&day_layer[i],
                        GRect(r.size.w - zero_width * (4 - i), section_height - text_offset, zero_width, section_height + text_offset),
                        true);
    }
    for (size_t i = 0; i < NUM_COLUMNS; ++i) {
        init_time_layer(&subday_layer[i],
                        GRect(r.size.w - zero_width * (4 - i), section_height * 2 - text_offset, zero_width, section_height + text_offset),
                        true);
    }

    switch (radix_point_style) {
    case DOT:
        snprintf(radix_str, 5, ".");
        break;
    case MAX_DIGIT:
        text_layer_set_font(&day_layer[3], fonts_load_custom_font(resource_get_handle(RESOURCE_ID_13)));
        snprintf(radix_str, 5, "\n\n\n%c", digit_to_radix_char(base, base - 1));
        break;
    }
    text_layer_set_text(&day_layer[3], radix_str);
}

static void handle_second_tick (AppContextRef ctx, PebbleTickEvent *t) {
    now = t->tick_time;
    draw_year(year_layer);
    draw_day(day_layer);
    draw_subday(subday_layer);
}

void pbl_main(void *params) {
    ticks_in_day = base * base * base * base;

    PebbleAppHandlers handlers = {
        .init_handler = &handle_init,
        .tick_info = {
            .tick_handler = &handle_second_tick,
            .tick_units = SECOND_UNIT
        }
    };
    app_event_loop(params, &handlers);
}
