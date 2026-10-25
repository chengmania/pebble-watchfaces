#pragma once

#include <pebble.h>

// Palette for one visual theme. Alternate themes (Toxic Green, Synth Pink)
// can be added by appending to the table in theme.c.
typedef struct {
  const char *name;
  GColor background;
  GColor primary;        // main digit / ring fill
  GColor primary_dim;    // secondary accents
  GColor shadow;         // offset drop shadow behind digits
  GColor track;          // unfilled portion of the rings
  GColor highlight;      // lightning core, inner digit highlight
  GColor warning;        // low battery, high heart rate
} ArcStormTheme;

typedef enum {
  ThemeArcStorm = 0,
  ThemeCount,
} ThemeId;

void theme_set(ThemeId id);
const ArcStormTheme *theme_current(void);
