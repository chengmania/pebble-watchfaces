#include "theme.h"

static const ArcStormTheme s_themes[ThemeCount] = {
  [ThemeArcStorm] = {
    .name = "Arc Storm",
    .background = { .argb = GColorBlackARGB8 },
    .primary = { .argb = GColorCyanARGB8 },
    .primary_dim = { .argb = GColorVividCeruleanARGB8 },
    .shadow = { .argb = GColorOxfordBlueARGB8 },
    .track = { .argb = GColorDukeBlueARGB8 },
    .highlight = { .argb = GColorWhiteARGB8 },
    .warning = { .argb = GColorShockingPinkARGB8 },
  },
};

static ThemeId s_current = ThemeArcStorm;

void theme_set(ThemeId id) {
  if (id < ThemeCount) {
    s_current = id;
  }
}

const ArcStormTheme *theme_current(void) {
  return &s_themes[s_current];
}
