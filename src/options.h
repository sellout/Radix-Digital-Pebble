#pragma once

// This is the numeric base to use for the display. It is written in base-10,
// because that’s what C wants to see.
unsigned int const base = 12;

enum radix_point_style {
    DOT,
    MAX_DIGIT
};

enum radix_point_style const radix_point_style = MAX_DIGIT;
