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

Window window;
TextLayer blank_layer;
TextLayer year_layer;
TextLayer day_layer;
TextLayer subday_layer;

static char year_str[5];
static char day_str[5];
static char subday_str[5];

PblTm *now;

int int_to_base_string(unsigned int base, int x, char *buffer, unsigned int pad) {
    buffer[0] = '\0';

    while (x > 0) {
        // shorten this to just map to ascii
        switch (x % base) {
        case  0: strcat(buffer, "0"); break;
        case  1: strcat(buffer, "1"); break;
        case  2: strcat(buffer, "2"); break;
        case  3: strcat(buffer, "3"); break;
        case  4: strcat(buffer, "4"); break;
        case  5: strcat(buffer, "5"); break;
        case  6: strcat(buffer, "6"); break;
        case  7: strcat(buffer, "7"); break;
        case  8: strcat(buffer, "8"); break;
        case  9: strcat(buffer, "9"); break;
        case 10: strcat(buffer, "a"); break;
        case 11: strcat(buffer, "b"); break;
        case 12: strcat(buffer, "c"); break;
        case 13: strcat(buffer, "d"); break;
        case 14: strcat(buffer, "e"); break;
        case 15: strcat(buffer, "f"); break;
        }
        x = x / base;                
    }

    while (strlen(buffer) < pad) strcat(buffer, "0");

    char t, *e = buffer + strlen(buffer);
    while (--e > buffer) {
        t = *buffer;
        *buffer++ = *e;
        *e = t;
    }

    return 0;
}

void draw_year(TextLayer *me) {
    int year = now->tm_year + 1900;
    int_to_base_string(base, year, year_str, 0);
    text_layer_set_text(me, year_str);
}

void draw_day(TextLayer *me) {
    int day = now->tm_yday;
    int_to_base_string(base, day, day_str, 0);
    strcat(day_str, ".");
    text_layer_set_text(me, day_str);
}

unsigned int const seconds_in_day = 86400;
unsigned int ticks_in_day;

void draw_subday(TextLayer *me) {
    unsigned int seconds_into_day = ((now->tm_hour * 60 + now->tm_min) * 60 + now->tm_sec);
    unsigned int subday = (seconds_into_day * ticks_in_day) / seconds_in_day;
    // 4.16666 // number of seconds in a day (86400) divided by number of sub-milli ticks in a day (20736)
    int_to_base_string(base, subday, subday_str, 4);
    text_layer_set_text(me, subday_str);
}

void init_time_layer(TextLayer *layer, GRect frame, bool dark) {
    GFont font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_51));
    text_layer_init(layer, frame);
    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_font(layer, font);
    text_layer_set_overflow_mode(layer, 2);
    text_layer_set_text_alignment(layer, GTextAlignmentRight);
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


    init_time_layer(&year_layer,
                    GRect(0, -10, r.size.w, section_height + 10),
                    false);
    init_time_layer(&day_layer,
                    GRect(0, section_height - 10, r.size.w, section_height + 10),
                    true);
    init_time_layer(&subday_layer,
                    GRect(0, section_height * 2 - 10, r.size.w, section_height + 10),
                    true);
}

static void handle_second_tick (AppContextRef ctx, PebbleTickEvent *t) {
    now = t->tick_time;
    draw_year(&year_layer);
    draw_day(&day_layer);
    draw_subday(&subday_layer);
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
