#pragma once

// This is the numeric base to use for various display components. It is written
// in base-10, because that’s what C wants to see. They can be set independently
// because it can aid in clarity. EG, someone trying to make this as much like a
// traditional clock as possible might set year_base to 10 (because that is the
// way we’re used to seeing the year), day_base to 7 (to match the week cycle),
// and subday_base to 12 (for somewhat complicated reasons).
unsigned int const year_base = 12;
unsigned int const day_base = 12;
unsigned int const subday_base = 12;

// How to display the radix point (the dot in 12.45). If the day and subday
// bases don't match, we fall back to DOT.
enum radix_point_style {
    DOT,      // show the traditional dot (or semicolon for base 12)
    UNARY,    // show a dot grid, where the number of dots = the base
    MAX_DIGIT // show a small version of the highest digit, EG, 1, 7, 9, b, and
              // f for binary, octal, decimal, dozenal, and hexidecimal.
};

enum radix_point_style const radix_point_style = UNARY;

enum time_display {
    NONE = 0, // don't display a clock
    LOCAL,    // standard local time
    SOLAR,    // local solar time, requires httpebble to fetch location
    UTC       // like GMT, but cooler, requires httpebble to fetch offset
};

enum time_display primary_clock_display = SOLAR;
enum time_display secondary_clock_display = UTC;

// Which symbols to use for digits larger than 9. If the option isn't available
// for a particular base, we fall back to SEQUENTIAL.
enum extended_numerals {
    SEQUENTIAL, // a, b, c, ...
    SELECTIVE,  // t, e for base 12
    CREATIVE    // *, # for base 12; \, /, :, *, ?, + for base 16
};

enum extended_numerals extended_numerals = SELECTIVE;
