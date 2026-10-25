#pragma once
#include <pebble.h>

// Geometry of a single 7-segment digit, scaled per screen.
#if defined(PBL_PLATFORM_DIORITE)
// diorite: 144x168
#define SEG_W 27          // digit width
#define SEG_H 46          // digit height
#define SEG_T 7           // segment thickness
#define SEG_GAP_PAIR 2    // gap between the two digits of a pair
#define SEG_GAP_COLON 5   // gap either side of the colon
#else
// emery: 200x228
#define SEG_W 38
#define SEG_H 76
#define SEG_T 10
#define SEG_GAP_PAIR 3
#define SEG_GAP_COLON 6
#endif

#define SEG_GAP 2         // gap trimmed off each end of a segment
#define SEG_COLON_W SEG_T // colon square size

// Width of the whole HH:MM block.
#define SEG_BLOCK_W (4 * SEG_W + 2 * SEG_GAP_PAIR + 2 * SEG_GAP_COLON + SEG_COLON_W)

#define SEG_BLANK (-1)

void seg_init(void);
void seg_deinit(void);

// origin is the top-left corner of the digit cell.
void seg_draw_digit(GContext *ctx, GPoint origin, int digit);

// origin is the top-left corner of the colon cell (SEG_COLON_W x SEG_H).
void seg_draw_colon(GContext *ctx, GPoint origin);
