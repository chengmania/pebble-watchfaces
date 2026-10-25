#pragma once

#include <pebble.h>

#include "theme.h"

// The rings frame the time as two side arcs, leaving a gap at the top and
// bottom of the circle for the slot rows.
#define RING_CENTER_X 100
#define RING_CENTER_Y 112
#define RING_OUTER_R 96
#define RING_OUTER_T 6
#define RING_INNER_R 85
#define RING_INNER_T 5

// Maps progress in [0, 1] onto the two arcs, for the spark animation.
int32_t rings_progress_angle(uint32_t numerator, uint32_t denominator);

// Draws one ring: dim track for the whole sweep, `fill` up to the progress.
void rings_draw(GContext *ctx, int radius, int thickness,
                uint32_t value, uint32_t max, GColor fill, GColor track,
                bool ticks);
