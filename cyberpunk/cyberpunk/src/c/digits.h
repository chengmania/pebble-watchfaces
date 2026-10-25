#pragma once

#include <pebble.h>

#include "theme.h"

// Geometry of one angular "cyber" digit cell.
#define DIGIT_W 28
#define DIGIT_H 56
#define DIGIT_T 8
#define DIGIT_GAP 4
#define COLON_W 10
#define TIME_BLOCK_W (4 * DIGIT_W + 2 * DIGIT_GAP + COLON_W + 2 * (DIGIT_GAP + 1))

// Per-draw glitch parameters. Zeroed out means a clean render.
typedef struct {
  int8_t slice_offset[4];  // horizontal shift, in px, for each digit
  int8_t row_offset[3];    // per-band tear: top, middle, bottom of every digit
  bool swap_colors;        // render in highlight/warning instead of primary
} DigitsGlitch;

void digits_init(void);
void digits_deinit(void);

// Draws HH:MM with the top-left of the block at `origin`.
// Horizontal extent of the ink a given time actually lights up, relative to
// the block origin, including the shadow and highlight passes. A leading "1"
// lights only its right-hand segments, so centring the cell box would push the
// visible time off to the right; callers centre on this instead.
void digits_time_ink_bounds(int hour, int minute, bool hide_leading_zero,
                            int *left, int *right);

// `hide_leading_zero` blanks the tens-of-hours digit (12h mode before 10:00).
void digits_draw_time(GContext *ctx, GPoint origin, int hour, int minute,
                      bool hide_leading_zero, const ArcStormTheme *theme,
                      const DigitsGlitch *glitch);
