#include "slots.h"

#define ICON_SIZE 14
#define ICON_TEXT_GAP 3
#define HIGH_HR_BPM 120
#define LOW_BATTERY_PCT 20

// ---------------------------------------------------------------- icons

static void prv_icon_bolt(GContext *ctx, GPoint o, GColor c) {
  static GPoint pts[7];
  static GPathInfo info = { .num_points = 7, .points = pts };
  pts[0] = GPoint(8, 0);
  pts[1] = GPoint(2, 8);
  pts[2] = GPoint(6, 8);
  pts[3] = GPoint(4, 14);
  pts[4] = GPoint(11, 5);
  pts[5] = GPoint(7, 5);
  pts[6] = GPoint(10, 0);
  GPath *p = gpath_create(&info);
  gpath_move_to(p, o);
  graphics_context_set_fill_color(ctx, c);
  gpath_draw_filled(ctx, p);
  gpath_destroy(p);
}

static void prv_icon_sun(GContext *ctx, GPoint o, GColor c) {
  const GPoint centre = GPoint(o.x + 7, o.y + 7);
  graphics_context_set_fill_color(ctx, c);
  graphics_fill_circle(ctx, centre, 4);
  graphics_context_set_stroke_color(ctx, c);
  for (int deg = 0; deg < 360; deg += 45) {
    const int32_t a = DEG_TO_TRIGANGLE(deg);
    const GPoint p1 = {
      .x = centre.x + (int16_t)(sin_lookup(a) * 5 / TRIG_MAX_RATIO),
      .y = centre.y - (int16_t)(cos_lookup(a) * 5 / TRIG_MAX_RATIO),
    };
    const GPoint p2 = {
      .x = centre.x + (int16_t)(sin_lookup(a) * 7 / TRIG_MAX_RATIO),
      .y = centre.y - (int16_t)(cos_lookup(a) * 7 / TRIG_MAX_RATIO),
    };
    graphics_draw_line(ctx, p1, p2);
  }
}

static void prv_icon_moon(GContext *ctx, GPoint o, GColor c, GColor bg) {
  graphics_context_set_fill_color(ctx, c);
  graphics_fill_circle(ctx, GPoint(o.x + 7, o.y + 7), 6);
  // Bite a disc out of the side to leave a crescent.
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_circle(ctx, GPoint(o.x + 10, o.y + 5), 6);
}

static void prv_icon_cloud(GContext *ctx, GPoint o, GColor c) {
  graphics_context_set_fill_color(ctx, c);
  graphics_fill_circle(ctx, GPoint(o.x + 5, o.y + 7), 4);
  graphics_fill_circle(ctx, GPoint(o.x + 9, o.y + 6), 3);
  graphics_fill_rect(ctx, GRect(o.x + 2, o.y + 7, 10, 4), 2, GCornersAll);
}

static void prv_icon_drops(GContext *ctx, GPoint o, GColor c, bool snow) {
  graphics_context_set_fill_color(ctx, c);
  for (int i = 0; i < 3; i++) {
    const int x = o.x + 3 + i * 4;
    if (snow) {
      graphics_fill_circle(ctx, GPoint(x, o.y + 12), 1);
    } else {
      graphics_fill_rect(ctx, GRect(x, o.y + 11, 1, 3), 0, GCornerNone);
    }
  }
}

static void prv_icon_weather(GContext *ctx, GPoint o, WeatherIcon id,
                             const ArcStormTheme *theme) {
  switch (id) {
    case WxClear:
      prv_icon_sun(ctx, o, theme->primary);
      break;
    case WxCloudy:
      prv_icon_cloud(ctx, o, theme->primary);
      break;
    case WxRain:
      prv_icon_cloud(ctx, o, theme->primary_dim);
      prv_icon_drops(ctx, o, theme->primary, false);
      break;
    case WxSnow:
      prv_icon_cloud(ctx, o, theme->primary_dim);
      prv_icon_drops(ctx, o, theme->highlight, true);
      break;
    case WxStorm:
      prv_icon_cloud(ctx, o, theme->primary_dim);
      prv_icon_bolt(ctx, GPoint(o.x + 1, o.y + 3), theme->highlight);
      break;
    case WxFog:
    default:
      graphics_context_set_stroke_color(ctx, theme->primary_dim);
      for (int i = 0; i < 3; i++) {
        const int y = o.y + 4 + i * 3;
        graphics_draw_line(ctx, GPoint(o.x + 1, y), GPoint(o.x + 12, y));
      }
      break;
  }
}

// `scale` is 0..255 and grows the heart for the pulse animation.
static void prv_icon_heart(GContext *ctx, GPoint o, GColor c, uint8_t scale) {
  const int grow = (scale * 2) / 255;
  const int r = 3 + grow;
  const GPoint left = GPoint(o.x + 4, o.y + 5);
  const GPoint right = GPoint(o.x + 4 + r + 1, o.y + 5);
  graphics_context_set_fill_color(ctx, c);
  graphics_fill_circle(ctx, left, r);
  graphics_fill_circle(ctx, right, r);

  static GPoint pts[3];
  static GPathInfo info = { .num_points = 3, .points = pts };
  pts[0] = GPoint(left.x - r, left.y + 1);
  pts[1] = GPoint(right.x + r, right.y + 1);
  pts[2] = GPoint((left.x + right.x) / 2, o.y + 8 + r * 2);
  GPath *p = gpath_create(&info);
  gpath_draw_filled(ctx, p);
  gpath_destroy(p);
}

static void prv_icon_steps(GContext *ctx, GPoint o, GColor c) {
  graphics_context_set_fill_color(ctx, c);
  // Two offset footprints: a sole with a separate heel pad.
  graphics_fill_rect(ctx, GRect(o.x, o.y + 1, 5, 7), 2, GCornersAll);
  graphics_fill_rect(ctx, GRect(o.x + 1, o.y + 9, 3, 3), 1, GCornersAll);
  graphics_fill_rect(ctx, GRect(o.x + 8, o.y + 3, 5, 7), 2, GCornersAll);
  graphics_fill_rect(ctx, GRect(o.x + 9, o.y + 11, 3, 3), 1, GCornersAll);
}

static void prv_icon_battery(GContext *ctx, GPoint o, const ArcStormTheme *theme,
                             BatteryChargeState state) {
  const GRect body = GRect(o.x, o.y + 3, 18, 9);
  const bool low = state.charge_percent <= LOW_BATTERY_PCT && !state.is_charging;
  const GColor accent = low ? theme->warning : theme->primary;

  graphics_context_set_stroke_color(ctx, theme->primary_dim);
  graphics_draw_rect(ctx, body);
  graphics_context_set_fill_color(ctx, theme->primary_dim);
  graphics_fill_rect(ctx, GRect(body.origin.x + body.size.w, o.y + 6, 2, 3), 0, GCornerNone);

  // Four pips rather than a bar, for the HUD read.
  graphics_context_set_fill_color(ctx, accent);
  const int pips = (state.charge_percent + 24) / 25;
  for (int i = 0; i < pips && i < 4; i++) {
    graphics_fill_rect(ctx, GRect(body.origin.x + 2 + i * 4, o.y + 5, 3, 5), 0, GCornerNone);
  }

  if (state.is_charging) {
    prv_icon_bolt(ctx, GPoint(o.x + 5, o.y + 1), theme->highlight);
  }
}

// ---------------------------------------------------------------- text

static void prv_format_steps(char *buf, size_t size, uint32_t steps) {
  if (steps < 1000) {
    snprintf(buf, size, "%d", (int)steps);
  } else {
    snprintf(buf, size, "%d,%03d", (int)(steps / 1000), (int)(steps % 1000));
  }
}

static void prv_format_clock(char *buf, size_t size, int minutes, bool clock_24h) {
  int h = minutes / 60;
  const int m = minutes % 60;
  if (clock_24h) {
    snprintf(buf, size, "%d:%02d", h, m);
  } else {
    h = h % 12;
    if (h == 0) {
      h = 12;
    }
    snprintf(buf, size, "%d:%02d", h, m);
  }
}

// Draws an icon plus label as one unit, aligned within `rect`.
static void prv_draw_pair_font(GContext *ctx, GRect rect, GTextAlignment align,
                               const char *text, GColor text_color, GFont font,
                               int icon_w, void (*draw_icon)(GContext *, GPoint, void *),
                               void *icon_ctx) {
  const GSize text_size = text ? graphics_text_layout_get_content_size(
                                     text, font, GRect(0, 0, rect.size.w, rect.size.h),
                                     GTextOverflowModeFill, GTextAlignmentLeft)
                               : GSize(0, 0);
  const int gap = (icon_w && text) ? ICON_TEXT_GAP : 0;
  const int total = icon_w + gap + text_size.w;

  int x = rect.origin.x;
  if (align == GTextAlignmentCenter) {
    x += (rect.size.w - total) / 2;
  } else if (align == GTextAlignmentRight) {
    x += rect.size.w - total;
  }

  if (draw_icon) {
    draw_icon(ctx, GPoint(x, rect.origin.y + (rect.size.h - ICON_SIZE) / 2), icon_ctx);
  }
  if (text) {
    graphics_context_set_text_color(ctx, text_color);
    graphics_draw_text(ctx, text, font,
                       GRect(x + icon_w + gap, rect.origin.y - 2, text_size.w + 4, rect.size.h),
                       GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  }
}

static void prv_draw_pair(GContext *ctx, GRect rect, GTextAlignment align,
                          const char *text, GColor text_color,
                          int icon_w, void (*draw_icon)(GContext *, GPoint, void *),
                          void *icon_ctx) {
  prv_draw_pair_font(ctx, rect, align, text, text_color,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     icon_w, draw_icon, icon_ctx);
}

// Trampolines so prv_draw_pair can stay generic over the icon functions.
typedef struct {
  const ArcStormTheme *theme;
  const SlotData *data;
} IconCtx;

static void prv_tramp_weather(GContext *ctx, GPoint o, void *c) {
  IconCtx *ic = c;
  prv_icon_weather(ctx, o, ic->data->wx_icon, ic->theme);
}

static void prv_tramp_sun(GContext *ctx, GPoint o, void *c) {
  IconCtx *ic = c;
  prv_icon_sun(ctx, o, ic->theme->primary);
}

static void prv_tramp_moon(GContext *ctx, GPoint o, void *c) {
  IconCtx *ic = c;
  prv_icon_moon(ctx, o, ic->theme->primary_dim, ic->theme->background);
}

static void prv_tramp_heart(GContext *ctx, GPoint o, void *c) {
  IconCtx *ic = c;
  const bool high = ic->data->have_hr && ic->data->hr >= HIGH_HR_BPM;
  prv_icon_heart(ctx, o, high ? ic->theme->warning : ic->theme->primary,
                 ic->data->heart_phase);
}

static void prv_tramp_steps(GContext *ctx, GPoint o, void *c) {
  IconCtx *ic = c;
  prv_icon_steps(ctx, o, ic->theme->primary);
}

static void prv_tramp_battery(GContext *ctx, GPoint o, void *c) {
  IconCtx *ic = c;
  prv_icon_battery(ctx, o, ic->theme, ic->data->battery);
}

void slots_draw(GContext *ctx, GRect rect, SlotType type, GTextAlignment align,
                const SlotData *data, const ArcStormTheme *theme) {
  char buf[16];
  IconCtx ic = { .theme = theme, .data = data };

  switch (type) {
    case SlotWeather:
      if (data->have_weather) {
        snprintf(buf, sizeof(buf), "%d°", data->temp);
      } else {
        snprintf(buf, sizeof(buf), "--°");
      }
      prv_draw_pair(ctx, rect, align, buf, theme->primary, ICON_SIZE,
                    prv_tramp_weather, &ic);
      break;

    case SlotSunrise:
      if (data->have_sun) {
        prv_format_clock(buf, sizeof(buf), data->sunrise_min, data->clock_24h);
      } else {
        snprintf(buf, sizeof(buf), "--:--");
      }
      prv_draw_pair(ctx, rect, align, buf, theme->primary_dim, ICON_SIZE,
                    prv_tramp_sun, &ic);
      break;

    case SlotSunset:
      if (data->have_sun) {
        prv_format_clock(buf, sizeof(buf), data->sunset_min, data->clock_24h);
      } else {
        snprintf(buf, sizeof(buf), "--:--");
      }
      prv_draw_pair(ctx, rect, align, buf, theme->primary_dim, ICON_SIZE,
                    prv_tramp_moon, &ic);
      break;

    case SlotSunriseSunset: {
      GRect half = rect;
      half.size.w = rect.size.w / 2;
      slots_draw(ctx, half, SlotSunrise, align, data, theme);
      half.origin.x += half.size.w;
      slots_draw(ctx, half, SlotSunset, align, data, theme);
      break;
    }

    case SlotHeartRate:
      if (data->have_hr) {
        snprintf(buf, sizeof(buf), "%d", data->hr);
      } else {
        snprintf(buf, sizeof(buf), "--");
      }
      prv_draw_pair(ctx, rect, align, buf,
                    (data->have_hr && data->hr >= HIGH_HR_BPM) ? theme->warning
                                                               : theme->primary,
                    ICON_SIZE, prv_tramp_heart, &ic);
      break;

    case SlotSteps:
      prv_format_steps(buf, sizeof(buf), data->steps);
      prv_draw_pair(ctx, rect, align, buf, theme->primary, ICON_SIZE,
                    prv_tramp_steps, &ic);
      break;

    case SlotBattery:
      prv_draw_pair(ctx, rect, align, NULL, theme->primary, 20,
                    prv_tramp_battery, &ic);
      break;

    case SlotDate:
      prv_draw_pair_font(ctx, rect, align, data->date_short, theme->primary_dim,
                         fonts_get_system_font(FONT_KEY_GOTHIC_18), 0, NULL, NULL);
      break;

    case SlotOff:
    default:
      break;
  }
}
