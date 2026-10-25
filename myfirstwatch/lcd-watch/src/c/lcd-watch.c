#include <pebble.h>

// ---------------------------------------------------------------------------
// Themes
// ---------------------------------------------------------------------------

// Unlit segments are stippled back to the panel colour. Emery's palette is
// two bits per channel, so there is often no darker shade of a given hue to
// drop to; thinning the stipple darkens a ghost without shifting its colour.
typedef enum {
  GHOST_SOLID = 0,   // every pixel
  GHOST_HALF,        // checkerboard
  GHOST_QUARTER,     // one pixel in four
} GhostDensity;

typedef struct {
  const char *name;
  GColor background;  // frame / outer background
  GColor panel;       // panel fill behind the digits
  GColor lit;         // lit segment
  GColor ghost;       // unlit ("ghost") segment
  GColor label;       // printed legends (slot names, city) - not a segment
  uint8_t ghost_density; // GhostDensity: how much of an unlit segment is painted
} LcdTheme;

// Values are persisted and sent from the config page, so existing entries
// keep their numbers; new themes are appended.
typedef enum {
  THEME_CLASSIC_GREY = 0,
  THEME_RETRO_AMBER,
  THEME_CLASSIC_GREY_INVERTED,
  THEME_RETRO_AMBER_INVERTED,
  THEME_LCD_GREEN,
  THEME_LCD_GREEN_INVERTED,
  THEME_BLUE_BACKLIGHT,
  THEME_BLUE_BACKLIGHT_INVERTED,
  THEME_RED_LED,
  THEME_RED_LED_INVERTED,
  THEME_COUNT,
} ThemeId;

static LcdTheme s_themes[THEME_COUNT];

static void prv_themes_init(void) {
  s_themes[THEME_CLASSIC_GREY] = (LcdTheme) {
    .name = "Classic Grey",
    .background = GColorBlack,
    .panel = GColorWhite,
    .lit = GColorBlack,
    .ghost = GColorLightGray,
    .label = GColorDarkGray,
    .ghost_density = GHOST_HALF,
  };
  s_themes[THEME_RETRO_AMBER] = (LcdTheme) {
    .name = "Retro Amber",
    .background = GColorBlack,
    .panel = GColorBlack,
    .lit = GColorChromeYellow,
    .ghost = GColorWindsorTan,
    .label = GColorWindsorTan,
    .ghost_density = GHOST_QUARTER,
  };
  // Inverted: black panel, lit segments in white.
  s_themes[THEME_CLASSIC_GREY_INVERTED] = (LcdTheme) {
    .name = "Classic Grey Inverted",
    .background = GColorWhite,
    .panel = GColorBlack,
    .lit = GColorWhite,
    .ghost = GColorDarkGray,
    .label = GColorLightGray,
    .ghost_density = GHOST_HALF,
  };
  // Inverted: amber panel, lit segments in black.
  s_themes[THEME_RETRO_AMBER_INVERTED] = (LcdTheme) {
    .name = "Retro Amber Inverted",
    .background = GColorChromeYellow,
    .panel = GColorChromeYellow,
    .lit = GColorBlack,
    .ghost = GColorWindsorTan,
    .label = GColorWindsorTan,
    .ghost_density = GHOST_QUARTER,
  };
  // The grey-green panel of a classic calculator watch.
  s_themes[THEME_LCD_GREEN] = (LcdTheme) {
    .name = "LCD Green",
    .background = GColorDarkGreen,
    .panel = GColorMintGreen,
    .lit = GColorBlack,
    .ghost = GColorMayGreen,
    .label = GColorDarkGreen,
    .ghost_density = GHOST_HALF,
  };
  s_themes[THEME_LCD_GREEN_INVERTED] = (LcdTheme) {
    .name = "LCD Green Inverted",
    .background = GColorMintGreen,
    .panel = GColorDarkGreen,
    .lit = GColorMintGreen,
    .ghost = GColorMayGreen,
    .label = GColorMayGreen,
    .ghost_density = GHOST_HALF,
  };
  // Backlit: cyan segments on deep blue, the way an EL backlight looks.
  s_themes[THEME_BLUE_BACKLIGHT] = (LcdTheme) {
    .name = "Blue Backlight",
    .background = GColorOxfordBlue,
    .panel = GColorOxfordBlue,
    .lit = GColorCyan,
    .ghost = GColorCobaltBlue,
    .label = GColorPictonBlue,
    .ghost_density = GHOST_QUARTER,
  };
  s_themes[THEME_BLUE_BACKLIGHT_INVERTED] = (LcdTheme) {
    .name = "Blue Backlight Inverted",
    .background = GColorCyan,
    .panel = GColorCyan,
    .lit = GColorOxfordBlue,
    .ghost = GColorTiffanyBlue,
    .label = GColorCobaltBlue,
    .ghost_density = GHOST_QUARTER,
  };
  // Red LED, as on a seventies desk clock.
  s_themes[THEME_RED_LED] = (LcdTheme) {
    .name = "Red LED",
    .background = GColorBlack,
    .panel = GColorBlack,
    .lit = GColorRed,
    .ghost = GColorDarkCandyAppleRed,
    .label = GColorDarkCandyAppleRed,
    .ghost_density = GHOST_QUARTER,
  };
  s_themes[THEME_RED_LED_INVERTED] = (LcdTheme) {
    .name = "Red LED Inverted",
    .background = GColorRed,
    .panel = GColorRed,
    .lit = GColorBlack,
    .ghost = GColorDarkCandyAppleRed,
    .label = GColorBulgarianRose,
    .ghost_density = GHOST_QUARTER,
  };
}

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

typedef enum {
  CLOCK_FMT_SYSTEM = 0,   // follow the watch's own 12h/24h setting
  CLOCK_FMT_12H,
  CLOCK_FMT_24H,
} ClockFormat;

#define SLOT_COUNT 4   // A, B, C, D

#define SETTINGS_VERSION 3
#define PERSIST_KEY_SETTINGS 1
#define PERSIST_KEY_TEMP     2

typedef struct {
  uint8_t version;
  uint8_t theme;              // ThemeId
  uint8_t slots[SLOT_COUNT];  // SlotType
  uint8_t clock_format;       // ClockFormat
  uint8_t temp_celsius;
  int16_t tz_offset_min;      // minutes to add to UTC for the second clock
  char    tz_label[6];
  uint8_t battery_bar;        // show the battery bar across the top
  uint8_t separators;         // draw the frame lines between panels
} Settings;

// Version 1 had no battery_bar. Kept so an existing watch keeps its settings
// instead of being reset to defaults by the struct growing.
typedef struct {
  uint8_t version;
  uint8_t theme;
  uint8_t slots[SLOT_COUNT];
  uint8_t clock_format;
  uint8_t temp_celsius;
  int16_t tz_offset_min;
  char    tz_label[6];
} SettingsV1;

static Settings s_settings;

static const LcdTheme *prv_theme(void) {
  return &s_themes[s_settings.theme < THEME_COUNT ? s_settings.theme : THEME_CLASSIC_GREY];
}

static bool prv_is_24h(void) {
  switch (s_settings.clock_format) {
    case CLOCK_FMT_12H: return false;
    case CLOCK_FMT_24H: return true;
    case CLOCK_FMT_SYSTEM:
    default:            return clock_is_24h_style();
  }
}

// ---------------------------------------------------------------------------
// 7-segment drawing
// ---------------------------------------------------------------------------
//
//    aaaa      Segment bitmask order: a b c d e f g  (bit 0 .. bit 6)
//   f    b
//   f    b
//    gggg
//   e    c
//   e    c
//    dddd

#define SEG_A (1 << 0)
#define SEG_B (1 << 1)
#define SEG_C (1 << 2)
#define SEG_D (1 << 3)
#define SEG_E (1 << 4)
#define SEG_F (1 << 5)
#define SEG_G (1 << 6)

static const uint8_t s_digit_segs[10] = {
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,          // 0
  SEG_B | SEG_C,                                          // 1
  SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,                  // 2
  SEG_A | SEG_B | SEG_G | SEG_C | SEG_D,                  // 3
  SEG_F | SEG_G | SEG_B | SEG_C,                          // 4
  SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,                  // 5
  SEG_A | SEG_F | SEG_G | SEG_E | SEG_C | SEG_D,          // 6
  SEG_A | SEG_B | SEG_C,                                  // 7
  0x7F,                                                   // 8 (all seven)
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,          // 9
};

// Reusable tapered-hexagon path, mutated in place for every segment.
static GPoint s_seg_pts[6];
static GPathInfo s_seg_info = { 6, s_seg_pts };
static GPath *s_seg_path;

// Stipples a cell back towards the panel colour. Called once per glyph after
// its unlit segments are down and before its lit ones go on top, so segments
// that overlap (the diagonals cross the centre bars) cannot damage each other.
// The pattern keys off absolute pixel parity, so overlapping calls agree.
static void prv_stipple(GContext *ctx, GRect box, GhostDensity density, GColor panel) {
  if (density == GHOST_SOLID) {
    return;
  }

  graphics_context_set_stroke_color(ctx, panel);
  graphics_context_set_stroke_width(ctx, 1);
  for (int16_t py = box.origin.y; py < box.origin.y + box.size.h; py++) {
    if (density == GHOST_QUARTER && (py & 1)) {
      graphics_draw_line(ctx, GPoint(box.origin.x, py),
                         GPoint(box.origin.x + box.size.w - 1, py));
      continue;
    }
    for (int16_t px = box.origin.x + (py & 1); px < box.origin.x + box.size.w; px += 2) {
      graphics_draw_pixel(ctx, GPoint(px, py));
    }
  }
}

static void prv_fill_hex(GContext *ctx, const GPoint *pts) {
  for (int i = 0; i < 6; i++) {
    s_seg_path->points[i] = pts[i];
  }
  gpath_draw_filled(ctx, s_seg_path);
}

// Horizontal segment: pointed left and right ends.
static void prv_seg_h(GContext *ctx, int16_t x, int16_t y, int16_t len, int16_t t) {
  const int16_t h = t / 2;
  const GPoint pts[6] = {
    { x + h,       y },
    { x + len - h, y },
    { x + len,     y + h },
    { x + len - h, y + t },
    { x + h,       y + t },
    { x,           y + h },
  };
  prv_fill_hex(ctx, pts);
}

// Vertical segment: pointed top and bottom ends.
static void prv_seg_v(GContext *ctx, int16_t x, int16_t y, int16_t len, int16_t t) {
  const int16_t h = t / 2;
  const GPoint pts[6] = {
    { x + h, y },
    { x + t, y + h },
    { x + t, y + len - h },
    { x + h, y + len },
    { x,     y + len - h },
    { x,     y + h },
  };
  prv_fill_hex(ctx, pts);
}

// Draws the subset of a 7-segment digit named by `mask`, in one colour.
static void prv_seg7_paint(GContext *ctx, GRect box, uint8_t mask, int16_t t) {
  const int16_t x = box.origin.x, y = box.origin.y;
  const int16_t w = box.size.w, h = box.size.h;
  const int16_t gap = 1;                 // sliver between neighbouring segments
  const int16_t mid = (h - t) / 2;
  const int16_t v_len = mid + t;

  const struct { uint8_t bit; bool vertical; int16_t x, y, len; } segs[7] = {
    { SEG_A, false, x,         y,         w     },
    { SEG_B, true,  x + w - t, y,         v_len },
    { SEG_C, true,  x + w - t, y + mid,   v_len },
    { SEG_D, false, x,         y + h - t, w     },
    { SEG_E, true,  x,         y + mid,   v_len },
    { SEG_F, true,  x,         y,         v_len },
    { SEG_G, false, x,         y + mid,   w     },
  };

  for (int i = 0; i < 7; i++) {
    if (!(mask & segs[i].bit)) {
      continue;
    }
    if (segs[i].vertical) {
      prv_seg_v(ctx, segs[i].x, segs[i].y + gap, segs[i].len - 2 * gap, t);
    } else {
      prv_seg_h(ctx, segs[i].x + gap, segs[i].y, segs[i].len - 2 * gap, t);
    }
  }
}

// Draw one 7-segment digit inside `box`. Unlit segments are drawn in the ghost
// colour so the whole cell always reads as an LCD cell.
// Pass digit < 0 for a fully ghosted (blank) cell.
static void prv_draw_digit(GContext *ctx, GRect box, int digit, int16_t t) {
  const LcdTheme *theme = prv_theme();
  const uint8_t on = (digit >= 0 && digit <= 9) ? s_digit_segs[digit] : 0;
  const uint8_t off = 0x7F & ~on;

  graphics_context_set_fill_color(ctx, theme->ghost);
  prv_seg7_paint(ctx, box, off, t);

  prv_stipple(ctx, box, theme->ghost_density, theme->panel);

  graphics_context_set_fill_color(ctx, theme->lit);
  prv_seg7_paint(ctx, box, on, t);
}

// Colon between the hour and minute pair.
static void prv_draw_colon(GContext *ctx, int16_t cx, int16_t top, int16_t h, int16_t t) {
  graphics_context_set_fill_color(ctx, prv_theme()->lit);
  const int16_t r = t / 2;
  graphics_fill_rect(ctx, GRect(cx - r, top + h / 3 - r, t, t), 2, GCornersAll);
  graphics_fill_rect(ctx, GRect(cx - r, top + 2 * h / 3 - r, t, t), 2, GCornersAll);
}

// ---------------------------------------------------------------------------
// 14-segment alphanumeric drawing
// ---------------------------------------------------------------------------
//
//     -- A --
//    |\  |  /|
//    F H  I  J B
//    |  \ | / |
//     G1 --- G2
//    |  / | \ |
//    E K  L  M C
//    |/   |   \|
//     -- D --

#define Q_A  (1 << 0)
#define Q_B  (1 << 1)
#define Q_C  (1 << 2)
#define Q_D  (1 << 3)
#define Q_E  (1 << 4)
#define Q_F  (1 << 5)
#define Q_G1 (1 << 6)
#define Q_G2 (1 << 7)
#define Q_H  (1 << 8)
#define Q_I  (1 << 9)
#define Q_J  (1 << 10)
#define Q_K  (1 << 11)
#define Q_L  (1 << 12)
#define Q_M  (1 << 13)
#define Q_ALL 0x3FFF

// Marker used in slot strings for the degree sign: a single byte, so the
// renderer never has to decode UTF-8.
#define SEG_DEGREE '~'

static const uint16_t s_seg14_digits[10] = {
  Q_A | Q_B | Q_C | Q_D | Q_E | Q_F,                       // 0
  Q_B | Q_C,                                               // 1
  Q_A | Q_B | Q_G1 | Q_G2 | Q_E | Q_D,                     // 2
  Q_A | Q_B | Q_C | Q_D | Q_G1 | Q_G2,                     // 3
  Q_F | Q_G1 | Q_G2 | Q_B | Q_C,                           // 4
  Q_A | Q_F | Q_G1 | Q_G2 | Q_C | Q_D,                     // 5
  Q_A | Q_F | Q_E | Q_D | Q_C | Q_G1 | Q_G2,               // 6
  Q_A | Q_B | Q_C,                                         // 7
  Q_A | Q_B | Q_C | Q_D | Q_E | Q_F | Q_G1 | Q_G2,         // 8
  Q_A | Q_B | Q_C | Q_D | Q_F | Q_G1 | Q_G2,               // 9
};

static const uint16_t s_seg14_letters[26] = {
  Q_A | Q_B | Q_C | Q_E | Q_F | Q_G1 | Q_G2,               // A
  Q_A | Q_B | Q_C | Q_D | Q_G2 | Q_I | Q_L,                // B
  Q_A | Q_D | Q_E | Q_F,                                   // C
  Q_A | Q_B | Q_C | Q_D | Q_I | Q_L,                       // D
  Q_A | Q_D | Q_E | Q_F | Q_G1 | Q_G2,                     // E
  Q_A | Q_E | Q_F | Q_G1,                                  // F
  Q_A | Q_C | Q_D | Q_E | Q_F | Q_G2,                      // G
  Q_B | Q_C | Q_E | Q_F | Q_G1 | Q_G2,                     // H
  Q_A | Q_D | Q_I | Q_L,                                   // I
  Q_B | Q_C | Q_D | Q_E,                                   // J
  Q_E | Q_F | Q_G1 | Q_J | Q_M,                            // K
  Q_D | Q_E | Q_F,                                         // L
  Q_B | Q_C | Q_E | Q_F | Q_H | Q_J,                       // M
  Q_B | Q_C | Q_E | Q_F | Q_H | Q_M,                       // N
  Q_A | Q_B | Q_C | Q_D | Q_E | Q_F,                       // O
  Q_A | Q_B | Q_E | Q_F | Q_G1 | Q_G2,                     // P
  Q_A | Q_B | Q_C | Q_D | Q_E | Q_F | Q_M,                 // Q
  Q_A | Q_B | Q_E | Q_F | Q_G1 | Q_G2 | Q_M,               // R
  Q_A | Q_C | Q_D | Q_F | Q_G1 | Q_G2,                     // S
  Q_A | Q_I | Q_L,                                         // T
  Q_B | Q_C | Q_D | Q_E | Q_F,                             // U
  Q_E | Q_F | Q_J | Q_K,                                   // V
  Q_B | Q_C | Q_E | Q_F | Q_K | Q_M,                       // W
  Q_H | Q_J | Q_K | Q_M,                                   // X
  Q_H | Q_J | Q_L,                                         // Y
  Q_A | Q_D | Q_J | Q_K,                                   // Z
};

static uint16_t prv_seg14_mask(char c) {
  if (c >= '0' && c <= '9') {
    return s_seg14_digits[c - '0'];
  }
  if (c >= 'a' && c <= 'z') {
    c -= ('a' - 'A');
  }
  if (c >= 'A' && c <= 'Z') {
    return s_seg14_letters[c - 'A'];
  }
  switch (c) {
    case '-':          return Q_G1 | Q_G2;
    case '/':          return Q_J | Q_K;
    case SEG_DEGREE:   return Q_A | Q_B | Q_F | Q_G1 | Q_G2;   // ring in the top half
    case ' ':
    default:           return 0;
  }
}

// Draws the subset of a 14-segment cell named by `mask`, in one colour.
static void prv_seg14_paint(GContext *ctx, GRect box, uint16_t mask, int16_t t,
                            GColor color) {
  graphics_context_set_fill_color(ctx, color);
  const int16_t x = box.origin.x, y = box.origin.y;
  const int16_t w = box.size.w, h = box.size.h;
  const int16_t mid = (h - t) / 2;
  const int16_t v_len = mid + t;
  const int16_t cx = x + (w - t) / 2;       // left edge of the centre bars
  const int16_t half = w / 2;

  if (mask & Q_A)  { prv_seg_h(ctx, x, y, w, t); }
  if (mask & Q_D)  { prv_seg_h(ctx, x, y + h - t, w, t); }
  if (mask & Q_G1) { prv_seg_h(ctx, x, y + mid, half, t); }
  if (mask & Q_G2) { prv_seg_h(ctx, x + half, y + mid, w - half, t); }
  if (mask & Q_F)  { prv_seg_v(ctx, x, y, v_len, t); }
  if (mask & Q_B)  { prv_seg_v(ctx, x + w - t, y, v_len, t); }
  if (mask & Q_E)  { prv_seg_v(ctx, x, y + mid, v_len, t); }
  if (mask & Q_C)  { prv_seg_v(ctx, x + w - t, y + mid, v_len, t); }
  if (mask & Q_I)  { prv_seg_v(ctx, cx, y, v_len, t); }
  if (mask & Q_L)  { prv_seg_v(ctx, cx, y + mid, v_len, t); }

  // The diagonals are strokes rather than tapered polygons: at these cell
  // sizes a mitred diagonal is only a couple of pixels wide anyway.
  const uint16_t diagonals = Q_H | Q_J | Q_K | Q_M;
  if (!(mask & diagonals)) {
    return;
  }

  graphics_context_set_stroke_width(ctx, t > 2 ? t - 1 : t);
  graphics_context_set_stroke_color(ctx, color);

  if (mask & Q_H) { graphics_draw_line(ctx, GPoint(x + t, y + t), GPoint(cx, y + mid)); }
  if (mask & Q_J) { graphics_draw_line(ctx, GPoint(x + w - t, y + t), GPoint(cx + t, y + mid)); }
  if (mask & Q_K) { graphics_draw_line(ctx, GPoint(x + t, y + h - t), GPoint(cx, y + mid + t)); }
  if (mask & Q_M) { graphics_draw_line(ctx, GPoint(x + w - t, y + h - t),
                                       GPoint(cx + t, y + mid + t)); }
  graphics_context_set_stroke_width(ctx, 1);
}

static void prv_draw_seg_char(GContext *ctx, GRect box, char c, int16_t t) {
  const LcdTheme *theme = prv_theme();

  // A real display carries its colons and decimal points as their own fixed
  // cells rather than making them out of segments, so these are drawn as dots.
  if (c == ':' || c == '.') {
    const int16_t d = (t > 2) ? t : 2;
    const int16_t cx = box.origin.x + (box.size.w - d) / 2;
    graphics_context_set_fill_color(ctx, theme->lit);
    if (c == ':') {
      graphics_fill_rect(ctx, GRect(cx, box.origin.y + box.size.h / 3 - d / 2, d, d),
                         1, GCornersAll);
      graphics_fill_rect(ctx, GRect(cx, box.origin.y + 2 * box.size.h / 3 - d / 2, d, d),
                         1, GCornersAll);
    } else {
      graphics_fill_rect(ctx, GRect(cx, box.origin.y + box.size.h - d, d, d),
                         1, GCornersAll);
    }
    return;
  }

  const uint16_t on = prv_seg14_mask(c);
  const uint16_t off = Q_ALL & ~on;

  prv_seg14_paint(ctx, box, off, t, theme->ghost);
  prv_stipple(ctx, box, theme->ghost_density, theme->panel);
  prv_seg14_paint(ctx, box, on, t, theme->lit);
}

// Draws `text` as 14-segment cells, centred in `box`. Cells shrink from
// `max_cell_w` if the string would not otherwise fit.
static void prv_draw_seg_text(GContext *ctx, GRect box, const char *text,
                              int16_t max_cell_w, int16_t gap, int16_t t) {
  const int len = strlen(text);
  if (len <= 0) {
    return;
  }

  int16_t cell_w = (box.size.w - (len - 1) * gap) / len;
  if (cell_w > max_cell_w) {
    cell_w = max_cell_w;
  }
  if (cell_w < 4) {
    cell_w = 4;
  }

  const int16_t used = len * cell_w + (len - 1) * gap;
  const int16_t x0 = box.origin.x + (box.size.w - used) / 2;

  for (int i = 0; i < len; i++) {
    prv_draw_seg_char(ctx, GRect(x0 + i * (cell_w + gap), box.origin.y,
                                 cell_w, box.size.h), text[i], t);
  }
}

// ---------------------------------------------------------------------------
// Complication slots
// ---------------------------------------------------------------------------

typedef enum {
  SLOT_TEMP = 0,
  SLOT_STEPS,
  SLOT_DATE,
  SLOT_DAY,
  SLOT_BATTERY,
  SLOT_OFF,
  SLOT_HEART,
  SLOT_ACTIVE,
  SLOT_DISTANCE,
  SLOT_CALORIES,
  SLOT_SUNRISE,
  SLOT_SUNSET,
  SLOT_CONDITION,
  SLOT_TYPE_COUNT,
} SlotType;

// Live values behind the slots. Updated by their services / tick handler.
static char s_date_buf[12];     // "OCT 24"
static char s_day_buf[8];       // "SAT"
static int  s_battery_pct = -1;
static bool s_charging = false;
static int  s_steps = 0;
static bool s_steps_ok = false;
// Last reading from the phone, remembered together with the unit it arrived
// in so a unit change can be reflected before the next refresh lands.
static int  s_sunrise_min = -1;   // minutes after local midnight
static int  s_sunset_min = -1;
static char s_condition[10] = "";
static int  s_temp = 0;
static bool s_temp_ok = false;
static bool s_temp_is_celsius = false;

typedef enum {
  HEALTH_HEART = 0,
  HEALTH_ACTIVE_SECONDS,
  HEALTH_DISTANCE,
  HEALTH_CALORIES,
} LcdHealthMetric;

// Returns -1 when Health cannot answer, so every caller shows "-" rather than
// a confident zero.
static int prv_health_value(LcdHealthMetric which) {
#if defined(PBL_HEALTH)
  const time_t start = time_start_of_today();
  const time_t now = time(NULL);

  if (which == HEALTH_HEART) {
    if (!(health_service_metric_accessible(HealthMetricHeartRateBPM, start, now)
          & HealthServiceAccessibilityMaskAvailable)) {
      return -1;
    }
    return (int)health_service_peek_current_value(HealthMetricHeartRateBPM);
  }

  HealthMetric metric;
  switch (which) {
    case HEALTH_ACTIVE_SECONDS: metric = HealthMetricActiveSeconds; break;
    case HEALTH_DISTANCE:       metric = HealthMetricWalkedDistanceMeters; break;
    case HEALTH_CALORIES:       metric = HealthMetricActiveKCalories; break;
    default:                    return -1;
  }

  if (!(health_service_metric_accessible(metric, start, now)
        & HealthServiceAccessibilityMaskAvailable)) {
    return -1;
  }
  return (int)health_service_sum_today(metric);
#else
  (void)which;
  return -1;
#endif
}

static int prv_temp_in_current_unit(void) {
  if (s_temp_is_celsius == (bool)s_settings.temp_celsius) {
    return s_temp;
  }
  // Round half away from zero so the converted value matches what a fresh
  // reading in the new unit would show.
  return s_settings.temp_celsius ? ((s_temp - 32) * 5 + (s_temp >= 32 ? 4 : -4)) / 9
                                 : (s_temp * 9 + (s_temp >= 0 ? 2 : -2)) / 5 + 32;
}

// Fills in the slot's label and value.
// Returns false for a slot that should draw nothing.
static bool prv_slot_text(SlotType type, const char **label, char *value, size_t value_size) {
  switch (type) {
    case SLOT_TEMP:
      *label = "TEMP";
      if (s_temp_ok) {
        snprintf(value, value_size, "%d%c%c", prv_temp_in_current_unit(),
                 SEG_DEGREE, s_settings.temp_celsius ? 'C' : 'F');
      } else {
        snprintf(value, value_size, "-%c%c", SEG_DEGREE,
                 s_settings.temp_celsius ? 'C' : 'F');
      }
      return true;

    case SLOT_STEPS:
      *label = "STEPS";
      if (s_steps_ok) {
        // No thousands separator: a segment cell has no comma.
        snprintf(value, value_size, "%d", s_steps);
      } else {
        snprintf(value, value_size, "--");
      }
      return true;

    case SLOT_DATE:
      *label = "DATE";
      snprintf(value, value_size, "%s", s_date_buf);
      return true;

    case SLOT_DAY:
      *label = "DAY";
      snprintf(value, value_size, "%s", s_day_buf);
      return true;

    case SLOT_BATTERY:
      *label = "BATT";
      // No '%' either; the BATT legend already says what the number is.
      if (s_battery_pct >= 0) {
        snprintf(value, value_size, "%d", s_battery_pct);
      } else {
        snprintf(value, value_size, "-");
      }
      return true;

    case SLOT_HEART: {
      *label = "HEART";
      const int bpm = prv_health_value(HEALTH_HEART);
      if (bpm > 0) { snprintf(value, value_size, "%d", bpm); }
      else         { snprintf(value, value_size, "-"); }
      return true;
    }

    case SLOT_ACTIVE: {
      *label = "ACTIVE";
      const int mins = prv_health_value(HEALTH_ACTIVE_SECONDS) / 60;
      if (mins >= 0) { snprintf(value, value_size, "%d", mins); }
      else           { snprintf(value, value_size, "-"); }
      return true;
    }

    case SLOT_DISTANCE: {
      // Distance follows the temperature unit: imperial reader, imperial units.
      *label = s_settings.temp_celsius ? "DIST KM" : "DIST MI";
      const int metres = prv_health_value(HEALTH_DISTANCE);
      if (metres < 0) {
        snprintf(value, value_size, "-");
      } else if (s_settings.temp_celsius) {
        snprintf(value, value_size, "%d.%d", metres / 1000, (metres % 1000) / 100);
      } else {
        const int tenths = (metres * 10) / 1609;
        snprintf(value, value_size, "%d.%d", tenths / 10, tenths % 10);
      }
      return true;
    }

    case SLOT_CALORIES: {
      *label = "CALS";
      const int kcal = prv_health_value(HEALTH_CALORIES);
      if (kcal >= 0) { snprintf(value, value_size, "%d", kcal); }
      else           { snprintf(value, value_size, "-"); }
      return true;
    }

    case SLOT_SUNRISE:
    case SLOT_SUNSET: {
      const bool rise = (type == SLOT_SUNRISE);
      const int minutes = rise ? s_sunrise_min : s_sunset_min;
      *label = rise ? "SUNRISE" : "SUNSET";
      if (minutes < 0) {
        snprintf(value, value_size, "-");
      } else {
        int hour = minutes / 60;
        if (!prv_is_24h()) {
          hour = hour % 12;
          if (hour == 0) { hour = 12; }
        }
        snprintf(value, value_size, "%d:%02d", hour, minutes % 60);
      }
      return true;
    }

    case SLOT_CONDITION:
      *label = "SKY";
      snprintf(value, value_size, "%s", s_condition[0] ? s_condition : "-");
      return true;

    case SLOT_OFF:
    default:
      return false;
  }
}

#define SLOT_CELL_W   16
#define SLOT_CELL_GAP  2
#define SLOT_SEG_T     3

static void prv_draw_slot(GContext *ctx, GRect panel, SlotType type) {
  const LcdTheme *theme = prv_theme();

  graphics_context_set_fill_color(ctx, theme->panel);
  graphics_fill_rect(ctx, panel, 0, GCornerNone);

  const char *label = NULL;
  char value[12];
  if (!prv_slot_text(type, &label, value, sizeof(value))) {
    return;
  }

  // Label sits above the value, the way an LCD legend does.
  graphics_context_set_text_color(ctx, theme->label);
  graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(panel.origin.x, panel.origin.y, panel.size.w, 16),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  prv_draw_seg_text(ctx, GRect(panel.origin.x + 3, panel.origin.y + 17,
                               panel.size.w - 6, 20),
                    value, SLOT_CELL_W, SLOT_CELL_GAP, SLOT_SEG_T);
}

// ---------------------------------------------------------------------------
// Second clock
// ---------------------------------------------------------------------------

#define SUB_DIGIT_W   22
#define SUB_DIGIT_H   34
#define SUB_SEG_T      6
#define SUB_DIGIT_GAP  4
#define SUB_COLON_W   10
#define SUB_LABEL_W   46
#define SUB_AMPM_W    24

static int s_sub_hour_tens, s_sub_hour_ones, s_sub_min_tens, s_sub_min_ones;
static bool s_sub_pm;
static int s_sub_day_offset;   // -1, 0 or +1 relative to the local date

static void prv_draw_second_clock(GContext *ctx, GRect panel) {
  const LcdTheme *theme = prv_theme();
  const bool h24 = prv_is_24h();

  graphics_context_set_fill_color(ctx, theme->panel);
  graphics_fill_rect(ctx, panel, 0, GCornerNone);

  const int16_t block_w = 4 * SUB_DIGIT_W + 2 * SUB_DIGIT_GAP + SUB_COLON_W;
  const int16_t content_w = SUB_LABEL_W + block_w + (h24 ? 0 : SUB_AMPM_W);
  const int16_t left = panel.origin.x + (panel.size.w - content_w) / 2;
  const int16_t top = panel.origin.y + (panel.size.h - SUB_DIGIT_H) / 2;

  // City label reads as a printed legend rather than a lit segment.
  graphics_context_set_text_color(ctx, theme->label);
  graphics_draw_text(ctx, s_settings.tz_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(left, top + 5, SUB_LABEL_W - 6, 24),
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);

  if (s_sub_day_offset != 0) {
    graphics_context_set_text_color(ctx, theme->label);
    graphics_draw_text(ctx, s_sub_day_offset > 0 ? "+1" : "-1",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(left, top + SUB_DIGIT_H - 16, SUB_LABEL_W - 6, 16),
                       GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  }

  const int16_t dx = left + SUB_LABEL_W;
  const int16_t xs[4] = {
    dx,
    dx + SUB_DIGIT_W + SUB_DIGIT_GAP,
    dx + 2 * SUB_DIGIT_W + SUB_DIGIT_GAP + SUB_COLON_W,
    dx + 3 * SUB_DIGIT_W + 2 * SUB_DIGIT_GAP + SUB_COLON_W,
  };
  const int digits[4] = { s_sub_hour_tens, s_sub_hour_ones, s_sub_min_tens, s_sub_min_ones };

  for (int i = 0; i < 4; i++) {
    prv_draw_digit(ctx, GRect(xs[i], top, SUB_DIGIT_W, SUB_DIGIT_H), digits[i], SUB_SEG_T);
  }

  prv_draw_colon(ctx, xs[1] + SUB_DIGIT_W + SUB_COLON_W / 2, top, SUB_DIGIT_H, SUB_SEG_T);

  if (!h24) {
    graphics_context_set_text_color(ctx, theme->lit);
    graphics_draw_text(ctx, s_sub_pm ? "PM" : "AM", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(xs[3] + SUB_DIGIT_W, top + SUB_DIGIT_H - 20, SUB_AMPM_W, 18),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
}

// ---------------------------------------------------------------------------
// Main time
// ---------------------------------------------------------------------------

#define TIME_DIGIT_W  37
#define TIME_DIGIT_H  62
#define TIME_SEG_T     9
#define TIME_DIGIT_GAP  5   // between the two digits of a pair
#define TIME_COLON_W   12   // gap holding the colon
#define TIME_AMPM_W    22   // right gutter for the AM/PM marker

static int s_hour_tens, s_hour_ones, s_min_tens, s_min_ones;
static bool s_pm;

static void prv_draw_main_time(GContext *ctx, GRect panel) {
  const LcdTheme *theme = prv_theme();
  const bool h24 = prv_is_24h();

  graphics_context_set_fill_color(ctx, theme->panel);
  graphics_fill_rect(ctx, panel, 0, GCornerNone);

  const int16_t block_w = 4 * TIME_DIGIT_W + 2 * TIME_DIGIT_GAP + TIME_COLON_W;
  // In 24h mode there is no AM/PM marker, so the digits get the gutter back.
  const int16_t left = panel.origin.x + (panel.size.w - block_w - (h24 ? 0 : TIME_AMPM_W)) / 2;
  const int16_t top = panel.origin.y + (panel.size.h - TIME_DIGIT_H) / 2;

  const int16_t xs[SLOT_COUNT] = {
    left,
    left + TIME_DIGIT_W + TIME_DIGIT_GAP,
    left + 2 * TIME_DIGIT_W + TIME_DIGIT_GAP + TIME_COLON_W,
    left + 3 * TIME_DIGIT_W + 2 * TIME_DIGIT_GAP + TIME_COLON_W,
  };
  const int digits[4] = { s_hour_tens, s_hour_ones, s_min_tens, s_min_ones };

  for (int i = 0; i < 4; i++) {
    prv_draw_digit(ctx, GRect(xs[i], top, TIME_DIGIT_W, TIME_DIGIT_H), digits[i], TIME_SEG_T);
  }

  prv_draw_colon(ctx, xs[1] + TIME_DIGIT_W + TIME_COLON_W / 2, top, TIME_DIGIT_H, TIME_SEG_T);

  if (!h24) {
    graphics_context_set_text_color(ctx, theme->lit);
    graphics_draw_text(ctx, s_pm ? "PM" : "AM", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(xs[3] + TIME_DIGIT_W, top + TIME_DIGIT_H - 22, TIME_AMPM_W, 18),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
}

// ---------------------------------------------------------------------------
// Battery bar
// ---------------------------------------------------------------------------

#define BATT_CELLS      10
#define BATT_CELL_GAP    2
#define BATT_CELL_INSET  2   // panel shows through as a border around the cells
#define BATT_LOW_PCT    10   // at or below this the bar inverts as a warning

// A row of cells across the top, lit from the left in proportion to charge.
// Unlit cells use the theme's ghost treatment, so the bar reads as part of the
// same LCD as the digits.
static void prv_draw_battery_bar(GContext *ctx, GRect panel) {
  const LcdTheme *theme = prv_theme();
  const int pct = (s_battery_pct < 0) ? 0 : s_battery_pct;
  // Running flat inverts the whole strip, which is hard to miss and still
  // looks like something an LCD would do.
  const bool low = (s_battery_pct >= 0 && pct <= BATT_LOW_PCT && !s_charging);

  const GColor bg = low ? theme->lit : theme->panel;
  const GColor fg = low ? theme->panel : theme->lit;

  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, panel, 0, GCornerNone);

  // Charging gets an outline rather than a colour change, so it reads as
  // distinct from the low warning.
  if (s_charging) {
    graphics_context_set_stroke_color(ctx, theme->lit);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_rect(ctx, panel);
  }

  const int16_t cell_h = panel.size.h - 2 * BATT_CELL_INSET;
  const int16_t cell_y = panel.origin.y + BATT_CELL_INSET;
  const int16_t cell_w = (panel.size.w - (BATT_CELLS - 1) * BATT_CELL_GAP) / BATT_CELLS;
  const int16_t used = BATT_CELLS * cell_w + (BATT_CELLS - 1) * BATT_CELL_GAP;
  const int16_t x0 = panel.origin.x + (panel.size.w - used) / 2;

  // Round up, so any remaining charge keeps at least one cell lit.
  const int lit_cells = (pct * BATT_CELLS + 99) / 100;

  for (int i = 0; i < BATT_CELLS; i++) {
    const GRect cell = GRect(x0 + i * (cell_w + BATT_CELL_GAP), cell_y, cell_w, cell_h);
    const bool is_lit = (i < lit_cells);

    graphics_context_set_fill_color(ctx, is_lit ? fg : theme->ghost);
    graphics_fill_rect(ctx, cell, 0, GCornerNone);

    if (!is_lit) {
      prv_stipple(ctx, cell, theme->ghost_density, bg);
    }
  }
}

// ---------------------------------------------------------------------------
// Face layout
// ---------------------------------------------------------------------------
//
//  +---------+---------+
//  | slot A  | slot B  |   FACE_SLOT_H
//  +---------+---------+
//  |   main time    PM |   remainder
//  +-------------------+
//  | LON   03:42 AM    |   FACE_CLOCK2_H
//  +---------+---------+
//  | slot C  | slot D  |   FACE_SLOT_H
//  +---------+---------+
//
// The gaps between panels are the background colour, so they read as the
// frame between panels without any extra drawing.

#define FACE_MARGIN    3
#define FACE_GAP       3
#define FACE_SLOT_H   40
#define FACE_CLOCK2_H 48
#define FACE_BATT_H   11

static Window *s_window;
static Layer *s_face_layer;

static void prv_face_update(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  const LcdTheme *theme = prv_theme();

  graphics_context_set_fill_color(ctx, theme->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // The frame is made of the gaps between panels, so switching the separators
  // off is simply a matter of closing them up; the panels then meet edge to
  // edge and the face reads as one continuous display.
  const int16_t margin = s_settings.separators ? FACE_MARGIN : 0;
  const int16_t gap = s_settings.separators ? FACE_GAP : 0;

  const int16_t x = margin;
  const int16_t w = bounds.size.w - 2 * margin;
  const int16_t half_w = (w - gap) / 2;
  const int16_t right_x = x + w - half_w;   // keeps the right edge flush
  // The battery bar is optional, so the main time panel absorbs its row when
  // it is switched off.
  const bool show_batt = s_settings.battery_bar;
  const int16_t batt_h = show_batt ? FACE_BATT_H + gap : 0;
  const int16_t time_h = bounds.size.h - 2 * margin - 3 * gap
                         - 2 * FACE_SLOT_H - FACE_CLOCK2_H - batt_h;

  int16_t y = margin;
  if (show_batt) {
    prv_draw_battery_bar(ctx, GRect(x, y, w, FACE_BATT_H));
    y += FACE_BATT_H + gap;
  }

  prv_draw_slot(ctx, GRect(x, y, half_w, FACE_SLOT_H), s_settings.slots[0]);
  prv_draw_slot(ctx, GRect(right_x, y, half_w, FACE_SLOT_H), s_settings.slots[1]);

  y += FACE_SLOT_H + gap;
  prv_draw_main_time(ctx, GRect(x, y, w, time_h));

  y += time_h + gap;
  prv_draw_second_clock(ctx, GRect(x, y, w, FACE_CLOCK2_H));

  y += FACE_CLOCK2_H + gap;
  prv_draw_slot(ctx, GRect(x, y, half_w, FACE_SLOT_H), s_settings.slots[2]);
  prv_draw_slot(ctx, GRect(right_x, y, half_w, FACE_SLOT_H), s_settings.slots[3]);
}

// ---------------------------------------------------------------------------
// Data services
// ---------------------------------------------------------------------------

static void prv_mark_dirty(void) {
  if (s_face_layer) {
    layer_mark_dirty(s_face_layer);
  }
}

static void prv_upper(char *str) {
  for (; *str; str++) {
    if (*str >= 'a' && *str <= 'z') {
      *str -= ('a' - 'A');
    }
  }
}

static void prv_update_steps(void) {
#if defined(PBL_HEALTH)
  const time_t start = time_start_of_today();
  const HealthServiceAccessibilityMask mask =
      health_service_metric_accessible(HealthMetricStepCount, start, time(NULL));
  if (mask & HealthServiceAccessibilityMaskAvailable) {
    s_steps = (int)health_service_sum_today(HealthMetricStepCount);
    s_steps_ok = true;
  } else {
    s_steps_ok = false;
  }
#else
  s_steps_ok = false;
#endif
}

#if defined(PBL_HEALTH)
static void prv_health_handler(HealthEventType event, void *context) {
  // A significant update also covers the midnight reset of the daily total.
  if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
    prv_update_steps();
    prv_mark_dirty();
  }
}
#endif

static void prv_update_battery(BatteryChargeState state) {
  s_battery_pct = state.charge_percent;
  s_charging = state.is_charging || state.is_plugged;
  prv_mark_dirty();
}

// ---------------------------------------------------------------------------
// Settings storage and AppMessage
// ---------------------------------------------------------------------------

static void prv_update_time(void);

static void prv_settings_defaults(void) {
  s_settings = (Settings) {
    .version = SETTINGS_VERSION,
    .theme = THEME_CLASSIC_GREY,
    // Defaults from the spec: A = Temp, B = Steps, C = Day, D = Date.
    .slots = { SLOT_TEMP, SLOT_STEPS, SLOT_DAY, SLOT_DATE },
    .clock_format = CLOCK_FMT_SYSTEM,
    .temp_celsius = false,
    .tz_offset_min = 0,
    .tz_label = "UTC",
    .battery_bar = true,
    .separators = true,
  };
}

static void prv_settings_load(void) {
  prv_settings_defaults();

  if (!persist_exists(PERSIST_KEY_SETTINGS)) {
    return;
  }

  // Read into the largest layout we know about, then dispatch on what was
  // actually stored. A blob that matches neither known version is ignored
  // rather than reinterpreted, leaving the defaults in place.
  union {
    Settings v2;
    SettingsV1 v1;
  } stored;
  const int read = persist_read_data(PERSIST_KEY_SETTINGS, &stored, sizeof(stored));

  if (read == (int)sizeof(Settings)
      && stored.v2.version >= 2 && stored.v2.version <= SETTINGS_VERSION) {
    const uint8_t was = stored.v2.version;
    s_settings = stored.v2;
    if (was < 3) {
      // `separators` occupied padding in v2, so whatever is there is not a
      // real value.
      s_settings.separators = true;
    }
    s_settings.version = SETTINGS_VERSION;
  } else if (read == (int)sizeof(SettingsV1) && stored.v1.version == 1) {
    // Migrate: carry every v1 field across and default the new one.
    s_settings.theme = stored.v1.theme;
    memcpy(s_settings.slots, stored.v1.slots, sizeof(s_settings.slots));
    s_settings.clock_format = stored.v1.clock_format;
    s_settings.temp_celsius = stored.v1.temp_celsius;
    s_settings.tz_offset_min = stored.v1.tz_offset_min;
    memcpy(s_settings.tz_label, stored.v1.tz_label, sizeof(s_settings.tz_label));
    s_settings.version = SETTINGS_VERSION;
    APP_LOG(APP_LOG_LEVEL_INFO, "migrated settings v1 -> v%d", SETTINGS_VERSION);
  } else {
    return;
  }

  s_settings.tz_label[sizeof(s_settings.tz_label) - 1] = '\0';
}

typedef struct {
  int16_t value;
  uint8_t is_celsius;
} StoredTemp;

static void prv_temp_load(void) {
  StoredTemp stored;
  if (persist_exists(PERSIST_KEY_TEMP)
      && persist_read_data(PERSIST_KEY_TEMP, &stored, sizeof(stored)) == (int)sizeof(stored)) {
    s_temp = stored.value;
    s_temp_is_celsius = stored.is_celsius;
    s_temp_ok = true;
  }
}

static void prv_temp_save(void) {
  const StoredTemp stored = { .value = (int16_t)s_temp, .is_celsius = s_temp_is_celsius };
  persist_write_data(PERSIST_KEY_TEMP, &stored, sizeof(stored));
}

static void prv_settings_save(void) {
  persist_write_data(PERSIST_KEY_SETTINGS, &s_settings, sizeof(s_settings));
}

// Clay sends select values as strings; accept integers too so the watch does
// not depend on how a given config control is declared.
static bool prv_tuple_int(DictionaryIterator *iter, uint32_t key, int *out) {
  const Tuple *t = dict_find(iter, key);
  if (!t) {
    return false;
  }

  switch (t->type) {
    case TUPLE_CSTRING:
      *out = atoi(t->value->cstring);
      return true;
    case TUPLE_INT:
      *out = (t->length == 1) ? t->value->int8
           : (t->length == 2) ? t->value->int16
                              : (int)t->value->int32;
      return true;
    case TUPLE_UINT:
      *out = (t->length == 1) ? t->value->uint8
           : (t->length == 2) ? t->value->uint16
                              : (int)t->value->uint32;
      return true;
    default:
      return false;
  }
}

static void prv_apply_enum(DictionaryIterator *iter, uint32_t key, uint8_t *field, int count) {
  int value;
  if (prv_tuple_int(iter, key, &value) && value >= 0 && value < count) {
    *field = (uint8_t)value;
  }
}

// The phone loses its copy of the settings whenever the app is reinstalled,
// and Clay then opens the config page on its defaults - saving from that state
// would push defaults back and wipe what the user chose. The watch keeps its
// settings in persistent storage, so it answers a READY ping with the real
// values and the phone reseeds itself from them.
static void prv_send_settings_to_phone(void *data) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "outbox busy; not answering READY");
    return;
  }

  dict_write_uint8(out, MESSAGE_KEY_THEME, s_settings.theme);
  dict_write_uint8(out, MESSAGE_KEY_CLOCK_24H, s_settings.clock_format);
  dict_write_uint8(out, MESSAGE_KEY_UNIT, s_settings.temp_celsius);
  dict_write_uint8(out, MESSAGE_KEY_SLOT_A, s_settings.slots[0]);
  dict_write_uint8(out, MESSAGE_KEY_SLOT_B, s_settings.slots[1]);
  dict_write_uint8(out, MESSAGE_KEY_SLOT_C, s_settings.slots[2]);
  dict_write_uint8(out, MESSAGE_KEY_SLOT_D, s_settings.slots[3]);
  dict_write_uint8(out, MESSAGE_KEY_BATTERY_BAR, s_settings.battery_bar);
  dict_write_uint8(out, MESSAGE_KEY_SEPARATORS, s_settings.separators);
  // The label identifies the city; the phone maps it back to an IANA zone,
  // so the watch does not have to store a 30-character zone name.
  dict_write_cstring(out, MESSAGE_KEY_TZ_LABEL, s_settings.tz_label);

  app_message_outbox_send();
}

static void prv_inbox_dropped(AppMessageResult reason, void *context) {
  // Nothing to recover here: the phone resends on its next refresh, and the
  // watch keeps showing the last values it had.
  APP_LOG(APP_LOG_LEVEL_WARNING, "dropped inbound message, reason %d", (int)reason);
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  // A READY ping carries nothing else; answer it off the handler's stack.
  int ready;
  if (prv_tuple_int(iter, MESSAGE_KEY_READY, &ready) && ready) {
    app_timer_register(100, prv_send_settings_to_phone, NULL);
    return;
  }

  prv_apply_enum(iter, MESSAGE_KEY_THEME, &s_settings.theme, THEME_COUNT);
  prv_apply_enum(iter, MESSAGE_KEY_CLOCK_24H, &s_settings.clock_format, CLOCK_FMT_24H + 1);
  prv_apply_enum(iter, MESSAGE_KEY_UNIT, &s_settings.temp_celsius, 2);
  prv_apply_enum(iter, MESSAGE_KEY_BATTERY_BAR, &s_settings.battery_bar, 2);
  prv_apply_enum(iter, MESSAGE_KEY_SEPARATORS, &s_settings.separators, 2);

  const uint32_t slot_keys[SLOT_COUNT] = {
    MESSAGE_KEY_SLOT_A, MESSAGE_KEY_SLOT_B, MESSAGE_KEY_SLOT_C, MESSAGE_KEY_SLOT_D,
  };
  for (int i = 0; i < SLOT_COUNT; i++) {
    prv_apply_enum(iter, slot_keys[i], &s_settings.slots[i], SLOT_TYPE_COUNT);
  }

  // The offset and its label describe the same city, so they are applied
  // together or not at all - a rejected offset must not leave the second
  // clock labelled with a city it is no longer showing.
  // UTC-12:00 to UTC+14:00 covers every real zone.
  int offset;
  const bool has_offset = prv_tuple_int(iter, MESSAGE_KEY_TZ_OFFSET, &offset);
  if (has_offset && offset >= -12 * 60 && offset <= 14 * 60) {
    s_settings.tz_offset_min = (int16_t)offset;

    const Tuple *label = dict_find(iter, MESSAGE_KEY_TZ_LABEL);
    // `length` counts the terminating NUL, so > 1 means a non-empty string.
    if (label && label->type == TUPLE_CSTRING && label->length > 1) {
      strncpy(s_settings.tz_label, label->value->cstring, sizeof(s_settings.tz_label) - 1);
      s_settings.tz_label[sizeof(s_settings.tz_label) - 1] = '\0';
    }
  }

  // TEMP_UNIT states the unit of this reading, independently of the user's
  // display preference, so the two can differ without the reading being
  // misinterpreted.
  int temp;
  if (prv_tuple_int(iter, MESSAGE_KEY_TEMP, &temp)) {
    int temp_unit;
    s_temp = temp;
    s_temp_is_celsius = prv_tuple_int(iter, MESSAGE_KEY_TEMP_UNIT, &temp_unit)
                      ? (temp_unit != 0)
                      : (bool)s_settings.temp_celsius;
    s_temp_ok = true;
    prv_temp_save();
  }

  int sun;
  if (prv_tuple_int(iter, MESSAGE_KEY_SUNRISE, &sun) && sun >= 0 && sun < 24 * 60) {
    s_sunrise_min = sun;
  }
  if (prv_tuple_int(iter, MESSAGE_KEY_SUNSET, &sun) && sun >= 0 && sun < 24 * 60) {
    s_sunset_min = sun;
  }

  const Tuple *cond = dict_find(iter, MESSAGE_KEY_CONDITION);
  if (cond && cond->type == TUPLE_CSTRING && cond->length > 1) {
    strncpy(s_condition, cond->value->cstring, sizeof(s_condition) - 1);
    s_condition[sizeof(s_condition) - 1] = '\0';
  }

  prv_settings_save();

  // A format change moves digits between cells, so recompute before redrawing.
  prv_update_time();
  prv_mark_dirty();
}

// ---------------------------------------------------------------------------
// Time keeping
// ---------------------------------------------------------------------------

// Seconds to add to UTC to get local time, derived from the watch's own clock.
static int32_t prv_local_utc_offset(time_t now) {
  const struct tm *lt = localtime(&now);
  const int local_secs = lt->tm_hour * 3600 + lt->tm_min * 60 + lt->tm_sec;
  struct tm utc = *gmtime(&now);
  const int utc_secs = utc.tm_hour * 3600 + utc.tm_min * 60 + utc.tm_sec;
  int32_t diff = local_secs - utc_secs;
  if (diff >  12 * 3600) { diff -= 24 * 3600; }
  if (diff < -12 * 3600) { diff += 24 * 3600; }
  return diff;
}

// Splits an hour/minute into the four digit cells, applying the 12/24h rule.
// In 12h mode a leading zero hour is ghosted (-1) the way a real LCD does,
// rather than blanked out of the layout.
static void prv_split_hm(int hour24, int minute, int *h_tens, int *h_ones,
                         int *m_tens, int *m_ones, bool *pm) {
  const bool h24 = prv_is_24h();

  *pm = (hour24 >= 12);

  int hour = hour24;
  if (!h24) {
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }
  }

  *h_tens = (hour / 10 == 0 && !h24) ? -1 : hour / 10;
  *h_ones = hour % 10;
  *m_tens = minute / 10;
  *m_ones = minute % 10;
}

static void prv_update_time(void) {
  const time_t now = time(NULL);
  const struct tm *tick = localtime(&now);

  prv_split_hm(tick->tm_hour, tick->tm_min,
               &s_hour_tens, &s_hour_ones, &s_min_tens, &s_min_ones, &s_pm);

  // Second clock: UTC shifted by the offset, so no local DST leaks into it.
  const time_t sub_t = now + s_settings.tz_offset_min * 60;
  const struct tm *sub = gmtime(&sub_t);
  prv_split_hm(sub->tm_hour, sub->tm_min,
               &s_sub_hour_tens, &s_sub_hour_ones, &s_sub_min_tens, &s_sub_min_ones,
               &s_sub_pm);

  // Is that city on yesterday's or tomorrow's date? Comparing days-since-epoch
  // avoids the year-end wrap that comparing tm_yday would hit.
  const int local_day = (int)((now + prv_local_utc_offset(now)) / 86400);
  const int sub_day = (int)(sub_t / 86400);
  s_sub_day_offset = sub_day - local_day;
  if (s_sub_day_offset > 1)  { s_sub_day_offset = 1; }
  if (s_sub_day_offset < -1) { s_sub_day_offset = -1; }

  strftime(s_date_buf, sizeof(s_date_buf), "%b %d", tick);
  strftime(s_day_buf, sizeof(s_day_buf), "%a", tick);
  prv_upper(s_date_buf);
  prv_upper(s_day_buf);

  prv_mark_dirty();
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  prv_update_time();
}

// ---------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);

  window_set_background_color(window, prv_theme()->background);

  s_face_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_face_layer, prv_face_update);
  layer_add_child(root, s_face_layer);

  prv_update_time();
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_face_layer);
  s_face_layer = NULL;
}

static void prv_init(void) {
  prv_themes_init();
  prv_settings_load();
  prv_temp_load();
  s_seg_path = gpath_create(&s_seg_info);

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, false);

  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);

  battery_state_service_subscribe(prv_update_battery);
  prv_update_battery(battery_state_service_peek());

#if defined(PBL_HEALTH)
  health_service_events_subscribe(prv_health_handler, NULL);
#endif
  prv_update_steps();

  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  window_destroy(s_window);
  gpath_destroy(s_seg_path);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
