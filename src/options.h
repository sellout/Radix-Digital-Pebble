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

enum radix_point_style {
    DOT,      // show the traditional dot
    MAX_DIGIT // show a small version of the highest digit, EG, 1, 7, 9, b, and
              // f for binary, octal, decimal, dozenal, and hexidecimal.
              // Displays '?' if the bases for day and subday are not the same.
};

enum radix_point_style const radix_point_style = MAX_DIGIT;

bool use_local_solar_time = true;

// when using base 12, this displays t(en) and e(leven) in lieu of a and b.
bool use_alternative_dozenal_digits = true;
