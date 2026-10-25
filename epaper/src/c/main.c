#include <pebble.h>
#include "seg.h"
#include "icons.h"

#if defined(PBL_PLATFORM_DIORITE)

// ---- diorite: 144x168 ----
#define SCREEN_W 144
#define SCREEN_H 168
#define MARGIN 7

#define TIME_Y 18
#define AMPM_Y 0
#define AMPM_H 18

#define DAY_Y 62
#define DATE_Y 82
#define DATE_LINE_H 24

#define RULE_Y 106
#define RULE_H 2

#define ROWS_TOP 110
#define ROW_H 19

// The sun times need more room than diorite's right column has beside the
// weather, so they trade places with the world time.
#define SUN_ON_HR_ROW 1
#define SUN1_X 44
#define SUN2_X 93
#define SUN_TEXT_W 38
#define TEMP_W 36

#else

// ---- emery: 200x228 ----
#define SCREEN_W 200
#define SCREEN_H 228
#define MARGIN 8

#define TIME_Y 26
#define AMPM_Y 2
#define AMPM_H 24

#define DAY_Y 100
#define DATE_Y 128
#define DATE_LINE_H 32

#define RULE_Y 160
#define RULE_H 3

#define ROWS_TOP 166
#define ROW_H 20

#define SUN_ON_HR_ROW 0
#define SUN1_X 86
#define SUN2_X 139
#define SUN_TEXT_W 44
#define TEMP_W 52

#endif

#define ROW_TEXT_X (MARGIN + 22)

#define PERSIST_SETTINGS 1
#define PERSIST_WEATHER 2

#define DATE_FMT_LONG 0   // SUNDAY / OCTOBER 25
#define DATE_FMT_SHORT 1  // SUN 25 OCT

typedef struct {
  bool invert;
  bool units_f;       // true = Fahrenheit
  bool clock_24h;
  bool leading_zero;  // 24h mode only
  bool show_steps;
  bool show_battery;
  bool show_hr;
  bool show_wx;
  bool show_sun;
  uint8_t wx_refresh;  // minutes
  uint8_t date_fmt;
  // Appended fields: older persisted blobs are shorter and keep their defaults.
  bool show_world;
  int16_t world_offset;   // minutes from UTC
  char world_label[6];
  // Kept last so shorter blobs written by earlier builds still load: the
  // default is written in before the read, so they validate as current.
  uint8_t version;
} Settings;

#define SETTINGS_VERSION 1

typedef struct {
  bool valid;
  int16_t temp;        // whole degrees, in the unit it was fetched for
  uint8_t icon;        // WxIcon
  int16_t sunrise;     // minutes since midnight, -1 if unknown
  int16_t sunset;
  int32_t fetched_at;  // epoch of the last reading; 0 means never
  int16_t sun_yday;    // tm_yday the sun times belong to; -1 if unknown
  bool temp_is_f;      // unit the phone actually fetched `temp` in
} Weather;

// A temperature this old is worse than no temperature.
#define WX_STALE_SECS (6 * 3600)

static Window *s_window;
static Layer *s_canvas;

static Settings s_set;
static Weather s_wx;

// Ink/paper are swapped by the Invert setting.
static GColor s_ink;
static GColor s_paper;

static GFont s_font_date;
static GFont s_font_row;
static GFont s_font_small;

static char s_ampm[3];
static char s_day[16];
static char s_date[20];
static char s_steps[12];
static char s_hr[8];
static int s_batt_bars;
static bool s_charging;
static bool s_connected = true;

// ---------------------------------------------------------------- settings

static void prv_defaults(void) {
  s_set.invert = false;
  s_set.units_f = true;
  s_set.clock_24h = clock_is_24h_style();
  s_set.leading_zero = true;
  s_set.show_steps = true;
  s_set.show_battery = true;
  s_set.show_hr = true;
  s_set.show_wx = true;
  s_set.show_sun = true;
  s_set.wx_refresh = 30;
  s_set.date_fmt = DATE_FMT_LONG;
  s_set.version = SETTINGS_VERSION;
  s_set.show_world = true;
  s_set.world_offset = 0;
  strcpy(s_set.world_label, "UTC");
}

static void prv_apply_colors(void) {
  s_ink = s_set.invert ? GColorWhite : GColorBlack;
  s_paper = s_set.invert ? GColorBlack : GColorWhite;
  if (s_window) {
    window_set_background_color(s_window, s_paper);
    // Dirty the root too: the window background is drawn by the root layer,
    // so marking only our canvas can leave the old paper colour behind.
    layer_mark_dirty(window_get_root_layer(s_window));
  }
}

static void prv_load_settings(void) {
  prv_defaults();
  if (persist_exists(PERSIST_SETTINGS)) {
    persist_read_data(PERSIST_SETTINGS, &s_set, sizeof(s_set));
  }
  if (s_set.version != SETTINGS_VERSION) {
    // Layout we do not recognise: start clean rather than read garbage.
    prv_defaults();
  }

  s_wx.valid = false;
  s_wx.sunrise = -1;
  s_wx.sunset = -1;
  s_wx.fetched_at = 0;
  s_wx.sun_yday = -1;
  if (persist_exists(PERSIST_WEATHER)) {
    persist_read_data(PERSIST_WEATHER, &s_wx, sizeof(s_wx));
  }
  prv_apply_colors();
}

// ---------------------------------------------------------------- helpers

// strftime returns names in the watch's language, whose accented characters
// have no glyph in our trimmed font subset. The face is English-only by
// design, so the names come from here instead.
static const char *const kDays[7] = {
  "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"
};
static const char *const kDaysShort[7] = {
  "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};
static const char *const kMonths[12] = {
  "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
  "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
};
static const char *const kMonthsShort[12] = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

// The bundled font carries only [ ,.:0-9A-Z-] and the degree sign, so anything
// outside that would render as a hole. Drop it rather than show a gap.
static void prv_sanitize_label(char *s) {
  char *w = s;
  for (const char *r = s; *r; r++) {
    char c = *r;
    if (c >= 'a' && c <= 'z') c -= 32;
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
        || c == ' ' || c == '-' || c == '.') {
      *w++ = c;
    }
  }
  *w = '\0';
  if (s[0] == '\0') strcpy(s, "UTC");
}

static void prv_upcase(char *s) {
  for (; *s; s++) {
    if (*s >= 'a' && *s <= 'z') *s -= 32;
  }
}

// "8431" -> "8,431"
static void prv_comma(int value, char *out, size_t out_len) {
  char raw[12];
  snprintf(raw, sizeof(raw), "%d", value);
  int len = strlen(raw);
  size_t o = 0;
  for (int i = 0; i < len && o + 2 < out_len; i++) {
    if (i > 0 && (len - i) % 3 == 0) out[o++] = ',';
    out[o++] = raw[i];
  }
  out[o] = '\0';
}

// Minutes since midnight -> "6:58" / "18:12", honouring the clock setting.
static void prv_fmt_minutes(int mins, char *out, size_t out_len) {
  if (mins < 0) {
    strncpy(out, "--", out_len);
    return;
  }
  int h = (mins / 60) % 24;
  int m = mins % 60;
  if (!s_set.clock_24h) {
    h = h % 12;
    if (h == 0) h = 12;
  }
  if (s_set.clock_24h) {
    snprintf(out, out_len, "%02d:%02d", h, m);
  } else {
    snprintf(out, out_len, "%d:%02d", h, m);
  }
}

// The HR sensor cannot be switched off system-wide by an app; what a face can
// do is request a sampling period while it wants readings, and cancel that
// request (period 0) to leave the system on its own battery-saving schedule.
#define HR_SAMPLE_SECS 300

static void prv_apply_hr_monitor(void) {
  const bool ok = health_service_set_heart_rate_sample_period(
      s_set.show_hr ? HR_SAMPLE_SECS : 0);

  if (s_set.show_hr && !ok) {
    // Worth one line in the log: "--" otherwise looks identical to a sensor
    // that simply has not sampled yet.
    const time_t now = time(NULL);
    const HealthServiceAccessibilityMask mask =
        health_service_metric_accessible(HealthMetricHeartRateBPM, now, now);
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "heart rate unavailable (mask=%d): %s", (int)mask,
            (mask & HealthServiceAccessibilityMaskNoPermission)
                ? "heart rate monitoring is off in the watch's Health settings"
                : (mask & HealthServiceAccessibilityMaskNotSupported)
                    ? "this watch has no heart rate sensor"
                    : "no reading yet");
  }
}

// Secondary timezone, as a fixed offset from UTC.
static void prv_world_time(char *out, size_t out_len) {
  time_t now = time(NULL);
  struct tm *g = gmtime(&now);
  int mins = g->tm_hour * 60 + g->tm_min + s_set.world_offset;
  mins = ((mins % 1440) + 1440) % 1440;

  char hhmm[8];
  prv_fmt_minutes(mins, hhmm, sizeof(hhmm));
  if (s_set.clock_24h) {
    snprintf(out, out_len, "%s %s", s_set.world_label, hhmm);
  } else {
    // A secondary 12h time is ambiguous without a meridiem marker.
    snprintf(out, out_len, "%s %s%c", s_set.world_label, hhmm,
             (mins < 12 * 60) ? 'A' : 'P');
  }
}

static int prv_temp_display(void) {
  if (s_wx.temp_is_f == s_set.units_f) return s_wx.temp;
  return s_set.units_f ? (s_wx.temp * 9 / 5 + 32)
                       : ((s_wx.temp - 32) * 5 / 9);
}

static bool prv_wx_fresh(void) {
  return s_wx.valid && s_wx.fetched_at > 0
      && (int32_t)time(NULL) - s_wx.fetched_at < WX_STALE_SECS;
}

// Sun times are specific to one calendar day; yesterday's are not useful.
static bool prv_sun_fresh(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  return s_wx.sun_yday >= 0 && s_wx.sun_yday == t->tm_yday;
}

// Kept off the stack: 20 records is ~240 bytes.
static HealthMinuteData s_hr_history[20];

static int prv_read_hr(void) {
  int bpm = (int)health_service_peek_current_value(HealthMetricHeartRateBPM);
  if (bpm > 0) return bpm;

  // peek reads 0 between samples, so fall back to the most recent reading
  // recorded in the last twenty minutes.
  time_t now = time(NULL);
  time_t start = now - 20 * 60;
  time_t end = now;
  uint32_t n = health_service_get_minute_history(s_hr_history,
                                                 ARRAY_LENGTH(s_hr_history),
                                                 &start, &end);
  for (int i = (int)n - 1; i >= 0; i--) {
    if (!s_hr_history[i].is_invalid && s_hr_history[i].heart_rate_bpm > 0) {
      return s_hr_history[i].heart_rate_bpm;
    }
  }
  return 0;
}

static void prv_update_health(void) {
  time_t now = time(NULL);
  if (health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), now)
      == HealthServiceAccessibilityMaskAvailable) {
    prv_comma((int)health_service_sum_today(HealthMetricStepCount), s_steps, sizeof(s_steps));
  } else {
    strcpy(s_steps, "--");
  }

  if (!s_set.show_hr) {
    strcpy(s_hr, "--");
  } else {
    int bpm = prv_read_hr();
    if (bpm > 0) {
      snprintf(s_hr, sizeof(s_hr), "%d", bpm);
    } else {
      strcpy(s_hr, "--");
    }
  }
}

static void prv_update_battery(void) {
  BatteryChargeState st = battery_state_service_peek();
  int bars = (st.charge_percent * 4 + 50) / 100;
  if (bars < 0) bars = 0;
  if (bars > 4) bars = 4;
  s_batt_bars = bars;
  s_charging = st.is_charging || st.is_plugged;
}

// ---------------------------------------------------------------- drawing

static void prv_draw_time(GContext *ctx, struct tm *t) {
  int hour = t->tm_hour;

  s_ampm[0] = '\0';
  if (!s_set.clock_24h) {
    strcpy(s_ampm, (hour < 12) ? "AM" : "PM");
    hour = hour % 12;
    if (hour == 0) hour = 12;
  }

  int d[4];
  d[0] = hour / 10;
  d[1] = hour % 10;
  d[2] = t->tm_min / 10;
  d[3] = t->tm_min % 10;

  // Blank the tens digit rather than showing a leading zero.
  if (d[0] == 0 && (!s_set.clock_24h || !s_set.leading_zero)) {
    d[0] = SEG_BLANK;
  }

  // Centre the HH:MM block, so both screens stay symmetric.
  const int x0 = (SCREEN_W - SEG_BLOCK_W) / 2;
  const int colon_x = x0 + 2 * SEG_W + SEG_GAP_PAIR + SEG_GAP_COLON;
  const int kx[4] = {
    x0,
    x0 + SEG_W + SEG_GAP_PAIR,
    colon_x + SEG_COLON_W + SEG_GAP_COLON,
    colon_x + SEG_COLON_W + SEG_GAP_COLON + SEG_W + SEG_GAP_PAIR,
  };

  graphics_context_set_fill_color(ctx, s_ink);
  for (int i = 0; i < 4; i++) {
    seg_draw_digit(ctx, GPoint(kx[i], TIME_Y), d[i]);
  }
  seg_draw_colon(ctx, GPoint(colon_x, TIME_Y));

  if (s_ampm[0]) {
    graphics_context_set_text_color(ctx, s_ink);
    graphics_draw_text(ctx, s_ampm, s_font_small,
                       GRect(0, AMPM_Y, SCREEN_W - MARGIN, AMPM_H),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
  }
}

static void prv_draw_date(GContext *ctx, struct tm *t) {
  GFont font = s_font_date;
  graphics_context_set_text_color(ctx, s_ink);

  if (s_set.date_fmt == DATE_FMT_SHORT) {
    snprintf(s_date, sizeof(s_date), "%s %d %s",
             kDaysShort[t->tm_wday], t->tm_mday, kMonthsShort[t->tm_mon]);
    graphics_draw_text(ctx, s_date, font,
                       GRect(MARGIN, (DAY_Y + DATE_Y) / 2, SCREEN_W - 2 * MARGIN, DATE_LINE_H),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  } else {
    snprintf(s_day, sizeof(s_day), "%s", kDays[t->tm_wday]);
    snprintf(s_date, sizeof(s_date), "%s %d", kMonths[t->tm_mon], t->tm_mday);

    graphics_draw_text(ctx, s_day, font,
                       GRect(MARGIN, DAY_Y, SCREEN_W - 2 * MARGIN, DATE_LINE_H),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, s_date, font,
                       GRect(MARGIN, DATE_Y, SCREEN_W - 2 * MARGIN, DATE_LINE_H),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }

  graphics_context_set_fill_color(ctx, s_ink);
  graphics_fill_rect(ctx, GRect(MARGIN, RULE_Y, SCREEN_W - 2 * MARGIN, RULE_H), 0, GCornerNone);
}

// Right-hand element of the heart-rate row, drawn at row top `y`.
static void prv_draw_world(GContext *ctx, int y) {
  char world[20];
  prv_world_time(world, sizeof(world));
  graphics_draw_text(ctx, world, s_font_small,
                     GRect(SCREEN_W - MARGIN - 96, y, 96, 20),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
}

static void prv_draw_sun(GContext *ctx, int y) {
  char rise[8], set[8];
  const bool fresh = prv_sun_fresh();
  prv_fmt_minutes(fresh ? s_wx.sunrise : -1, rise, sizeof(rise));
  prv_fmt_minutes(fresh ? s_wx.sunset : -1, set, sizeof(set));

  graphics_context_set_fill_color(ctx, s_ink);
  icon_draw(ctx, &ICON_ARROW_UP, GPoint(SUN1_X, y + 5));
  graphics_draw_text(ctx, rise, s_font_small,
                     GRect(SUN1_X + ICON_ARROW_UP.w + 2, y, SUN_TEXT_W, 20),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  icon_draw(ctx, &ICON_ARROW_DOWN, GPoint(SUN2_X, y + 5));
  graphics_draw_text(ctx, set, s_font_small,
                     GRect(SUN2_X + ICON_ARROW_DOWN.w + 2, y, SUN_TEXT_W, 20),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void prv_draw_rows(GContext *ctx) {
  GFont font = s_font_row;
  graphics_context_set_fill_color(ctx, s_ink);
  graphics_context_set_text_color(ctx, s_ink);

  // On the narrow screen the sun times ride with the heart rate and the world
  // time sits beside the weather; on the wide one it is the other way round.
  const bool sun_here = SUN_ON_HR_ROW;
  const bool hr_right = s_set.show_sun && sun_here;
  const bool wx_right = s_set.show_sun && !sun_here;

  int y = ROWS_TOP;

  // Row: steps on the left, battery on the right. Either half can be hidden.
  if (s_set.show_steps || s_set.show_battery) {
    if (s_set.show_steps) {
      icon_draw(ctx, &ICON_STEPS, GPoint(MARGIN, y + 2));
      graphics_draw_text(ctx, s_steps, font, GRect(ROW_TEXT_X, y - 4, 110, 24),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    }
    int rx = SCREEN_W - MARGIN;
    if (s_set.show_battery) {
      icon_draw_battery(ctx, GPoint(rx - BATT_W, y + 4), s_batt_bars, s_charging);
      rx -= BATT_W + 4;
      if (s_charging) rx -= ICON_BOLT.w + 3;
    }
    if (!connection_service_peek_pebble_app_connection()) {
      icon_draw(ctx, &ICON_BT_OFF, GPoint(rx - ICON_BT_OFF.w, y + 3));
    }
    y += ROW_H;
  }

  // Row: heart rate on the left.
  if (s_set.show_hr || (sun_here ? hr_right : s_set.show_world)) {
    if (s_set.show_hr) {
      icon_draw(ctx, &ICON_HEART, GPoint(MARGIN, y + 2));
      graphics_draw_text(ctx, s_hr, font, GRect(ROW_TEXT_X, y - 4, 70, 24),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    }
    if (sun_here) {
      if (hr_right) prv_draw_sun(ctx, y);
    } else if (s_set.show_world) {
      prv_draw_world(ctx, y);
    }
    y += ROW_H;
  }

  // Row: weather on the left.
  if (s_set.show_wx || (sun_here ? s_set.show_world : wx_right)) {
    char buf[32];
    if (s_set.show_wx) {
      if (prv_wx_fresh()) {
        icon_draw(ctx, icon_for_weather(s_wx.icon), GPoint(MARGIN, y + 2));
        snprintf(buf, sizeof(buf), "%d\xc2\xb0%c", prv_temp_display(),
                 s_set.units_f ? 'F' : 'C');
      } else {
        strcpy(buf, "--");
      }
      graphics_draw_text(ctx, buf, font, GRect(ROW_TEXT_X, y - 4, TEMP_W, 24),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    }
    if (sun_here) {
      if (s_set.show_world) prv_draw_world(ctx, y);
    } else if (wx_right) {
      prv_draw_sun(ctx, y);
    }
    y += ROW_H;
  }
}

static void prv_canvas_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_fill_color(ctx, s_paper);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  prv_draw_time(ctx, t);
  prv_draw_date(ctx, t);
  prv_draw_rows(ctx);
}

// ---------------------------------------------------------------- services

// The phone cannot be relied on to keep a timer alive across Bluetooth drops
// or a PebbleKit JS restart, so the watch decides when a refresh is due and
// asks for one. This also makes the Refresh setting authoritative.
static time_t s_last_request;

static void prv_request_weather(bool force) {
  if (!connection_service_peek_pebble_app_connection()) return;

  const time_t now = time(NULL);
  if (!force && now - s_last_request < 120) return;  // never hammer the phone

  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) != APP_MSG_OK) return;
  dict_write_uint8(out, MESSAGE_KEY_FETCH, 1);
  if (app_message_outbox_send() == APP_MSG_OK) {
    s_last_request = now;
  }
}

static bool prv_weather_due(void) {
  const int mins = s_set.wx_refresh ? s_set.wx_refresh : 30;
  return (int32_t)time(NULL) - s_wx.fetched_at >= mins * 60;
}

static void prv_connection_handler(bool connected) {
  s_connected = connected;
  if (connected) prv_request_weather(true);
  layer_mark_dirty(s_canvas);
}

static void prv_inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "inbox dropped: 0x%x", reason);
}

static void prv_outbox_failed(DictionaryIterator *it, AppMessageResult reason,
                              void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "outbox failed: 0x%x", reason);
}

static void prv_health_handler(HealthEventType event, void *context) {
  prv_update_health();
  layer_mark_dirty(s_canvas);
}

static void prv_battery_handler(BatteryChargeState state) {
  prv_update_battery();
  layer_mark_dirty(s_canvas);
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (prv_weather_due()) {
    prv_request_weather(false);
  }
  layer_mark_dirty(s_canvas);
}

// Clay sends toggles as integers but every <select> as a string ("0", "30",
// "-480"), so a tuple has to be read by its actual type and width. Reading
// ->int32 on a string yields its first bytes, which is never what we want.
static int32_t prv_tuple_int(const Tuple *t, int32_t fallback) {
  if (!t) return fallback;
  switch (t->type) {
    case TUPLE_CSTRING:
      return (int32_t)atoi(t->value->cstring);
    case TUPLE_INT:
      switch (t->length) {
        case 1: return t->value->int8;
        case 2: return t->value->int16;
        default: return t->value->int32;
      }
    case TUPLE_UINT:
      switch (t->length) {
        case 1: return t->value->uint8;
        case 2: return t->value->uint16;
        default: return (int32_t)t->value->uint32;
      }
    default:
      return fallback;
  }
}

static int32_t prv_read_int(DictionaryIterator *it, uint32_t key, int32_t current) {
  return prv_tuple_int(dict_find(it, key), current);
}

static bool prv_read_bool(DictionaryIterator *it, uint32_t key, bool current) {
  return prv_read_int(it, key, current ? 1 : 0) != 0;
}

static void prv_inbox_received(DictionaryIterator *it, void *context) {
  bool wx_changed = false;

  s_set.invert       = prv_read_bool(it, MESSAGE_KEY_INVERT, s_set.invert);
  s_set.units_f      = prv_read_bool(it, MESSAGE_KEY_UNIT, s_set.units_f);
  s_set.clock_24h    = prv_read_bool(it, MESSAGE_KEY_CLOCK_24H, s_set.clock_24h);
  s_set.leading_zero = prv_read_bool(it, MESSAGE_KEY_LEADING_ZERO, s_set.leading_zero);
  s_set.show_steps   = prv_read_bool(it, MESSAGE_KEY_SHOW_STEPS, s_set.show_steps);
  s_set.show_battery = prv_read_bool(it, MESSAGE_KEY_SHOW_BATTERY, s_set.show_battery);
  s_set.show_hr      = prv_read_bool(it, MESSAGE_KEY_SHOW_HR, s_set.show_hr);
  s_set.show_wx      = prv_read_bool(it, MESSAGE_KEY_SHOW_WX, s_set.show_wx);
  s_set.show_sun     = prv_read_bool(it, MESSAGE_KEY_SHOW_SUN, s_set.show_sun);

  s_set.wx_refresh = (uint8_t)prv_read_int(it, MESSAGE_KEY_WX_REFRESH, s_set.wx_refresh);
  s_set.date_fmt   = (uint8_t)prv_read_int(it, MESSAGE_KEY_DATE_FMT, s_set.date_fmt);

  Tuple *t;

  s_set.show_world   = prv_read_bool(it, MESSAGE_KEY_SHOW_WORLD, s_set.show_world);
  s_set.world_offset = (int16_t)prv_read_int(it, MESSAGE_KEY_WORLD_OFFSET, s_set.world_offset);

  t = dict_find(it, MESSAGE_KEY_WORLD_LABEL);
  if (t && t->value->cstring[0]) {
    strncpy(s_set.world_label, t->value->cstring, sizeof(s_set.world_label) - 1);
    s_set.world_label[sizeof(s_set.world_label) - 1] = '\0';
    prv_sanitize_label(s_set.world_label);
  }

  time_t now = time(NULL);
  t = dict_find(it, MESSAGE_KEY_TEMP);
  if (t) {
    s_wx.temp = (int16_t)prv_tuple_int(t, s_wx.temp);
    s_wx.valid = true;
    s_wx.fetched_at = (int32_t)now;
    // The phone states the unit it fetched in alongside the reading; if it is
    // an older payload without one, assume it matches the current setting.
    s_wx.temp_is_f = s_set.units_f;
    wx_changed = true;
  }
  t = dict_find(it, MESSAGE_KEY_WX_ICON);
  if (t) { s_wx.icon = (uint8_t)prv_tuple_int(t, s_wx.icon); wx_changed = true; }
  t = dict_find(it, MESSAGE_KEY_SUNRISE);
  if (t) { s_wx.sunrise = (int16_t)prv_tuple_int(t, s_wx.sunrise); wx_changed = true; }
  t = dict_find(it, MESSAGE_KEY_SUNSET);
  if (t) {
    s_wx.sunset = (int16_t)prv_tuple_int(t, s_wx.sunset);
    s_wx.sun_yday = localtime(&now)->tm_yday;
    wx_changed = true;
  }

  persist_write_data(PERSIST_SETTINGS, &s_set, sizeof(s_set));
  if (wx_changed) {
    persist_write_data(PERSIST_WEATHER, &s_wx, sizeof(s_wx));
  }

  prv_apply_colors();
  prv_apply_hr_monitor();
  prv_update_health();
  layer_mark_dirty(s_canvas);
}

// ---------------------------------------------------------------- lifecycle

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(window_layer));
  layer_set_update_proc(s_canvas, prv_canvas_update);
  layer_add_child(window_layer, s_canvas);
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_canvas);
}

static void prv_init(void) {
  prv_load_settings();
  seg_init();

#if defined(PBL_PLATFORM_DIORITE)
  s_font_date  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PLEX_20));
  s_font_row   = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PLEX_16));
  s_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PLEX_14));
#else
  s_font_date  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PLEX_28));
  s_font_row   = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PLEX_20));
  s_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PLEX_16));
#endif

  s_window = window_create();
  window_set_background_color(s_window, s_paper);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, false);

  prv_apply_hr_monitor();
  prv_update_health();
  prv_update_battery();

  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
  health_service_events_subscribe(prv_health_handler, NULL);
  battery_state_service_subscribe(prv_battery_handler);

  s_connected = connection_service_peek_pebble_app_connection();
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = prv_connection_handler,
  });

  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_failed(prv_outbox_failed);
  app_message_open(256, 64);

  if (prv_weather_due()) {
    prv_request_weather(true);
  }
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  health_service_set_heart_rate_sample_period(0);
  health_service_events_unsubscribe();
  connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  window_destroy(s_window);
  fonts_unload_custom_font(s_font_date);
  fonts_unload_custom_font(s_font_row);
  fonts_unload_custom_font(s_font_small);
  seg_deinit();
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
