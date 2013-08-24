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
    // keep these around so we can avoid redrawing when possible
    int prev_year;
    int prev_day;
    int prev_subday;
};

struct clock primary_clock;
struct clock secondary_clock;

Layer radix_layer;
char radix_str[5];

bool require_http = false;

PblTm *now;

int text_offset = 8;
int zero_width = 30; // space for each column, probably "0", a commonly-occuring wide character
int w_width = 36; // width of the widest character, probably "w"
int width_offset = 3; // (w_width - zero_width) / 2;

// This function finds the two factors closest to the square root, these are the
// factors that would be closest to a square if drawn on a cartesian plane. EG,
// 10 => 2 & 5, 12 => 3 & 4, 16 => 4 & 4
void find_central_factors(int number, int *factor1, int *factor2) {
    int second = number;
    for (int first = 1; first <= second; ++first) {
        second = number / first;
        if (first <= second && first * second == number) {
            *factor1 = first;
            *factor2 = second;
        }
    }
}

char digit_to_radix_char(unsigned int base, int digit) {
    if (digit < 10) 
        return (char)digit + '0';
    else if (extended_numerals == SELECTIVE && base == 12)
        return digit - 10 ? 'e' : 't';
    else if (extended_numerals == CREATIVE && base == 12)
        return digit - 10 ? '#' : '*';
    else if (extended_numerals == CREATIVE && base == 16) {
        switch (digit) {
        case 10: return '/';
        case 11: return '\\';
        case 12: return ':';
        case 13: return '*';
        case 14: return '?';
        default: return '+';
        }
    } else
        return (char)(digit - 10) + 'a';
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
    if (year != me->prev_year) {
        int_to_base_string(year_base, year, me->year_str, me->year_layer, 3, false);
        me->prev_year = year;
    }
}

void draw_day(struct clock *me, int day_offset) {
    int day = now->tm_yday + day_offset;
    if (day != me->prev_day) {
        int_to_base_string(day_base, day, me->day_str, me->day_layer, 2, false);
        me->prev_day = day;
    }
}

unsigned int ticks_in_day;

int draw_subday(struct clock *me, int second_offset) {
    int day_adjustment = 0;
    int seconds_into_day = (now->tm_hour * 60 + now->tm_min) * 60 + now->tm_sec + second_offset;
    if (seconds_into_day < 0) {
        seconds_into_day += 1 * DAYS;
        day_adjustment = -1;
    } else if (1 * DAYS <= seconds_into_day) {
        seconds_into_day -= 1 * DAYS;
        day_adjustment = 1;
    }
    int subday = (seconds_into_day * ticks_in_day) / (1 * DAYS);
    if (subday != me->prev_subday) {
        int_to_base_string(subday_base, subday, me->subday_str, me->subday_layer, 3, true);
        me->prev_subday = subday;
    }

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
    if (require_http && now->tm_sec == 0) update_LSP();


    if (primary_clock_display) {
        int p_second_offset = 0;
        switch (primary_clock_display) {
        case SOLAR: p_second_offset = current_lst_offset(); break;
        case UTC:   p_second_offset = current_utc_offset(); break;
        default:    break;
        }
        int p_day_offset = draw_subday(&primary_clock, p_second_offset);
        draw_day(&primary_clock, p_day_offset);
        draw_year(&primary_clock);
    }

    if (secondary_clock_display) {
        int s_second_offset = 0;
        switch (primary_clock_display) {
        case SOLAR: s_second_offset = current_lst_offset(); break;
        case UTC:   s_second_offset = current_utc_offset(); break;
        default:    break;
        }
        int s_day_offset = draw_subday(&secondary_clock, s_second_offset);
        draw_day(&secondary_clock, s_day_offset);
    }
}
int max_dot_radius(int count, int width) {
    return width / (3 * count - 1);
}

int unary_radix_dimension(int count, int radius) {
    return (3 * count - 1) * radius;
}

void draw_unary_radix(struct Layer *layer, GContext *ctx) {
    int a, b;
    find_central_factors(day_base, &a, &b);
    int radius = max_dot_radius(b, zero_width);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    for (int i = 0,
             y = (layer->bounds.size.h - unary_radix_dimension(a, radius)) * 2 / 3;
         i < a;
         ++i, y += 3 * radius) {
        for (int j = 0,
                 x = (layer->bounds.size.w - unary_radix_dimension(b, radius)) / 2;
             j < b;
             ++j, x += 3 * radius) {
            graphics_fill_circle(ctx, GPoint(x, y), radius);
        }
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

    for (size_t i = 0; i < NUM_COLUMNS; ++i) {
        init_time_layer(&primary_clock.year_layer[i],
                        GRect(r.size.w - zero_width * (4 - i) - width_offset, - text_offset, w_width, section_height + text_offset),
                        RESOURCE_ID_51,
                        false);
    }
    for (size_t i = 0; i < NUM_COLUMNS; ++i) {
        init_time_layer(&primary_clock.day_layer[i],
                        GRect(r.size.w - zero_width * (4 - i) - width_offset, section_height - text_offset, w_width, section_height + text_offset),
                        RESOURCE_ID_51,
                        true);
    }
    for (size_t i = 0; i < NUM_COLUMNS; ++i) {
        init_time_layer(&primary_clock.subday_layer[i],
                        GRect(r.size.w - zero_width * (4 - i) - width_offset, section_height * 2 - text_offset, w_width, section_height + text_offset),
                        RESOURCE_ID_51,
                        true);
    }

    layer_init(&radix_layer, primary_clock.day_layer[3].layer.frame);
    layer_add_child(&window.layer, &radix_layer);

    if (primary_clock_display) {
        switch (radix_point_style) {
        case UNARY:
            if (day_base == subday_base) {
                layer_set_update_proc(&radix_layer, draw_unary_radix);
                break;
            } // else fall-through
        case MAX_DIGIT:
            if (day_base == subday_base) {
                snprintf(radix_str, 5, "\n\n\n%c",
                         digit_to_radix_char(day_base, day_base - 1));
                text_layer_set_font(&primary_clock.day_layer[3],
                                    fonts_load_custom_font(resource_get_handle(RESOURCE_ID_13)));
                text_layer_set_text(&primary_clock.day_layer[3], radix_str);
                break;
            } // else fall-through
        case DOT:
            snprintf(radix_str, 5,
                     day_base == 12 && subday_base == 12 ? ";" : ".");
            text_layer_set_text(&primary_clock.day_layer[3], radix_str);
            break;
        }
    }

    if (secondary_clock_display) {
        int utc_section_height = section_height  / NUM_COLUMNS;
        int utc_text_offset = text_offset / NUM_COLUMNS;
        int utc_zero_width = r.size.w - zero_width * NUM_COLUMNS;

        for (size_t i = 0; i < NUM_COLUMNS; ++i) {
            init_time_layer(&secondary_clock.day_layer[i],
                            GRect(0, section_height + utc_section_height * i - utc_text_offset,
                                  utc_zero_width, utc_section_height + utc_text_offset),
                            RESOURCE_ID_13,
                            true);
        }
        for (size_t i = 0; i < NUM_COLUMNS; ++i) {
            init_time_layer(&secondary_clock.subday_layer[i],
                            GRect(0, section_height * 2 + utc_section_height * i - utc_text_offset,
                                  utc_zero_width, utc_section_height + utc_text_offset),
                            RESOURCE_ID_13,
                            true);
        }

        text_layer_set_text(&secondary_clock.day_layer[3], ".");
    }

    switch (primary_clock_display) {
    case UTC:
    case SOLAR:
        require_http = true;
        break;
    default:
        switch (secondary_clock_display) {
        case UTC:
        case SOLAR:
            require_http = true;
            break;
        default: break;
        }
    }

    if (require_http) init_LSP(-834577844);

    // draw immediately, so we don’t start with a blank face.
    PblTm right_now;
    now = &right_now;
    get_time(now);
    update_clock();
}

void handle_second_tick (AppContextRef ctx, PebbleTickEvent *t) {
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
