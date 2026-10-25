#include <pebble.h>

#include "digits.h"
#include "fx.h"
#include "rings.h"
#include "settings.h"
#include "slots.h"
#include "theme.h"

#define TIME_BLOCK_Y 84
#define DATE_Y 142
#define TOP_ROW_Y 2
#define BOTTOM_ROW_Y 198
#define LOW_BATTERY_PCT 20

static Window *s_window;
// Static chrome, the time, and the effects overlay are separate layers so an
// animation only has to repaint what is actually moving.
static Layer *s_bg_layer;
static Layer *s_digits_layer;
static Layer *s_fx_layer;

static char s_date_buf[16];
static char s_date_short_buf[12];
static struct tm s_now;

static SlotData s_data;

static void prv_update_time(void) {
  const time_t now = time(NULL);
  s_now = *localtime(&now);
  strftime(s_date_buf, sizeof(s_date_buf), "%a %b %d", &s_now);
  // Cyber HUDs shout; the stock strftime abbreviations do not.
  for (char *p = s_date_buf; *p; p++) {
    if (*p >= 'a' && *p <= 'z') {
      *p -= 32;
    }
  }
  strftime(s_date_short_buf, sizeof(s_date_short_buf), "%b %d", &s_now);
  for (char *p = s_date_short_buf; *p; p++) {
    if (*p >= 'a' && *p <= 'z') {
      *p -= 32;
    }
  }
  s_data.date = s_date_buf;
  s_data.date_short = s_date_short_buf;
  s_data.clock_24h = settings_is_24h();
}

static void prv_update_health(void) {
#if defined(PBL_HEALTH)
  const time_t now = time(NULL);
  if (health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), now)
      & HealthServiceAccessibilityMaskAvailable) {
    s_data.steps = (uint32_t)health_service_sum_today(HealthMetricStepCount);
  }
  if (health_service_metric_accessible(HealthMetricHeartRateBPM, now, now)
      & HealthServiceAccessibilityMaskAvailable) {
    const HealthValue bpm = health_service_peek_current_value(HealthMetricHeartRateBPM);
    s_data.have_hr = bpm > 0;
    s_data.hr = (int)bpm;
  } else {
    s_data.have_hr = false;
  }
#endif
}

// Lays the three slots of a row across the screen: left, centre, right.
static void prv_draw_row(GContext *ctx, int16_t width, int y, const SlotType *slots,
                         const ArcStormTheme *theme) {
  const int16_t slot_w = width / 3;
  const GTextAlignment aligns[3] = { GTextAlignmentLeft, GTextAlignmentCenter,
                                     GTextAlignmentRight };
  for (int i = 0; i < 3; i++) {
    const GRect r = GRect(4 + i * slot_w, y, slot_w - 8, SLOT_ROW_H);
    slots_draw(ctx, r, slots[i], aligns[i], &s_data, theme);
  }
}

// Either ring can be pointed at steps, at the battery, or switched off.
static void prv_draw_ring(GContext *ctx, RingSource source, int radius, int thickness,
                          bool ticks, const ArcStormTheme *theme) {
  const Settings *settings = settings_get();
  switch (source) {
    case RingSteps:
      rings_draw(ctx, radius, thickness, s_data.steps, settings->step_goal,
                 theme->primary, theme->track, ticks);
      break;
    case RingBattery: {
      const bool low = s_data.battery.charge_percent <= LOW_BATTERY_PCT;
      rings_draw(ctx, radius, thickness, s_data.battery.charge_percent, 100,
                 low ? theme->warning : theme->primary_dim, theme->track, ticks);
      break;
    }
    case RingOff:
    default:
      break;
  }
}

static void prv_bg_update(Layer *layer, GContext *ctx) {
  const ArcStormTheme *theme = theme_current();
  s_data.heart_phase = fx_heart_phase(s_data.have_hr ? s_data.hr : 60);
  const GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, theme->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  const Settings *settings = settings_get();
  prv_draw_ring(ctx, settings->ring_outer, RING_OUTER_R, RING_OUTER_T, true, theme);
  prv_draw_ring(ctx, settings->ring_inner, RING_INNER_R, RING_INNER_T, false, theme);

  graphics_context_set_text_color(ctx, theme->primary_dim);
  graphics_draw_text(ctx, s_date_buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(0, DATE_Y, bounds.size.w, 28), GTextOverflowModeFill,
                     GTextAlignmentCenter, NULL);

  prv_draw_row(ctx, bounds.size.w, TOP_ROW_Y, settings->top, theme);
  prv_draw_row(ctx, bounds.size.w, BOTTOM_ROW_Y, settings->bottom, theme);
}

static void prv_digits_update(Layer *layer, GContext *ctx) {
  const ArcStormTheme *theme = theme_current();
  const GRect bounds = layer_get_bounds(layer);

  const bool is_24h = settings_is_24h();
  int hour = s_now.tm_hour;
  if (!is_24h) {
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }
  }

  int ink_left, ink_right;
  digits_time_ink_bounds(hour, s_now.tm_min, !is_24h, &ink_left, &ink_right);
  const int block_x = (bounds.size.w - (ink_right - ink_left)) / 2 - ink_left;

  DigitsGlitch glitch;
  fx_fill_glitch(&glitch);
  digits_draw_time(ctx, GPoint(block_x, TIME_BLOCK_Y), hour, s_now.tm_min,
                   !is_24h, theme, &glitch);
}

static void prv_fx_update(Layer *layer, GContext *ctx) {
  fx_render(ctx, layer_get_bounds(layer), theme_current());
}

static void prv_redraw_all(void) {
  if (s_bg_layer) { layer_mark_dirty(s_bg_layer); }
  if (s_digits_layer) { layer_mark_dirty(s_digits_layer); }
}

static void prv_tap_handler(AccelAxisType axis, int32_t direction) {
  if (settings_get()->anim_mode == AnimCalm) {
    return;
  }
  fx_strike();
}

static void prv_tick(struct tm *tick_time, TimeUnits units_changed) {
  prv_update_time();
  prv_update_health();
  prv_redraw_all();

  // The spark runs along whatever the outer ring is actually showing.
  const Settings *settings = settings_get();
  if (settings->anim_mode != AnimCalm) {
    if (settings->ring_outer == RingSteps) {
      fx_spark(s_data.steps, settings->step_goal);
    } else if (settings->ring_outer == RingBattery) {
      fx_spark(s_data.battery.charge_percent, 100);
    }
  }
}

static void prv_battery_handler(BatteryChargeState state) {
  s_data.battery = state;
  fx_set_charging(state.is_charging && settings_get()->anim_mode != AnimCalm);
  prv_redraw_all();
}

#if defined(PBL_HEALTH)
static void prv_health_handler(HealthEventType event, void *context) {
  if (event != HealthEventSleepUpdate) {
    prv_update_health();
    prv_redraw_all();
  }
}
#endif

// Copies the persisted weather cache into the slot data used for drawing.
static void prv_apply_weather(void) {
  const WeatherCache *wx = weather_cache_get();
  const time_t age = time(NULL) - wx->updated;

  s_data.have_weather = wx->valid && age >= 0 && age < WEATHER_MAX_AGE_S;
  s_data.temp = wx->temp;
  s_data.wx_icon = (WeatherIcon)wx->wx_icon;
  s_data.have_sun = wx->have_sun && age >= 0 && age < SUN_MAX_AGE_S;
  s_data.sunrise_min = wx->sunrise_min;
  s_data.sunset_min = wx->sunset_min;
}

// Calm mode switches every effect off, including the ones that run on timers.
static void prv_apply_anim_settings(void) {
  const Settings *settings = settings_get();
  if (settings->anim_mode == AnimCalm) {
    // Calm mode also stops whatever happens to be mid-flight.
    fx_cancel_all();
    return;
  }
  fx_set_pulse_always(settings->hr_pulse == PulseAlways);
  fx_set_charging(s_data.battery.is_charging);
}

static SlotType prv_slot_from_int(int32_t value, SlotType fallback) {
  return (value >= 0 && value < SlotTypeCount) ? (SlotType)value : fallback;
}

// Settings arrive from the phone, so nothing in the dictionary is trusted to
// be in range; an out-of-range value leaves the current setting alone.
static int32_t prv_enum_or(int32_t value, int32_t count, int32_t fallback) {
  return (value >= 0 && value < count) ? value : fallback;
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  Settings *settings = settings_get();
  Tuple *t;

  if ((t = dict_find(iter, MESSAGE_KEY_THEME))) {
    settings->theme = (ThemeId)prv_enum_or(t->value->int32, ThemeCount, settings->theme);
    theme_set(settings->theme);
  }

  const uint32_t top_keys[3] = { MESSAGE_KEY_TOP_1, MESSAGE_KEY_TOP_2, MESSAGE_KEY_TOP_3 };
  const uint32_t bot_keys[3] = { MESSAGE_KEY_BOT_1, MESSAGE_KEY_BOT_2, MESSAGE_KEY_BOT_3 };
  for (int i = 0; i < 3; i++) {
    if ((t = dict_find(iter, top_keys[i]))) {
      settings->top[i] = prv_slot_from_int(t->value->int32, settings->top[i]);
    }
    if ((t = dict_find(iter, bot_keys[i]))) {
      settings->bottom[i] = prv_slot_from_int(t->value->int32, settings->bottom[i]);
    }
  }

  if ((t = dict_find(iter, MESSAGE_KEY_RING_OUTER))) {
    settings->ring_outer =
        (RingSource)prv_enum_or(t->value->int32, RingSourceCount, settings->ring_outer);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_RING_INNER))) {
    settings->ring_inner =
        (RingSource)prv_enum_or(t->value->int32, RingSourceCount, settings->ring_inner);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_STEP_GOAL)) && t->value->int32 > 0) {
    settings->step_goal = (uint32_t)t->value->int32;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_UNIT))) {
    settings->fahrenheit = t->value->int32 != 0;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_CLOCK_24H))) {
    settings->clock_format =
        (ClockFormat)prv_enum_or(t->value->int32, ClockFormatCount, settings->clock_format);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_ANIM_MODE))) {
    settings->anim_mode =
        (AnimMode)prv_enum_or(t->value->int32, AnimModeCount, settings->anim_mode);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_HR_PULSE))) {
    settings->hr_pulse =
        (HeartPulseMode)prv_enum_or(t->value->int32, HeartPulseModeCount, settings->hr_pulse);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_WX_REFRESH)) && t->value->int32 > 0) {
    settings->wx_refresh_min = (uint8_t)t->value->int32;
  }

  // Weather and sun times arrive on the same channel as the settings.
  WeatherCache *wx = weather_cache_get();
  bool wx_changed = false;
  if ((t = dict_find(iter, MESSAGE_KEY_TEMP))) {
    wx->temp = (int)t->value->int32;
    wx->valid = true;
    wx_changed = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_WX_ICON))) {
    wx->wx_icon = (uint8_t)t->value->int32;
    wx_changed = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_SUNRISE))) {
    wx->sunrise_min = (int16_t)t->value->int32;
    wx->have_sun = true;
    wx_changed = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_SUNSET))) {
    wx->sunset_min = (int16_t)t->value->int32;
    wx_changed = true;
  }
  if (wx_changed) {
    wx->updated = time(NULL);
    weather_cache_save();
    prv_apply_weather();
  }

  settings_save();
  prv_apply_anim_settings();
  prv_update_time();
  prv_redraw_all();
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  const GRect bounds = layer_get_bounds(window_layer);

  s_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_bg_layer, prv_bg_update);
  layer_add_child(window_layer, s_bg_layer);

  s_digits_layer = layer_create(bounds);
  layer_set_update_proc(s_digits_layer, prv_digits_update);
  layer_add_child(window_layer, s_digits_layer);

  s_fx_layer = layer_create(bounds);
  layer_set_update_proc(s_fx_layer, prv_fx_update);
  layer_add_child(window_layer, s_fx_layer);

  fx_init(s_bg_layer, s_digits_layer, s_fx_layer);
}

static void prv_window_unload(Window *window) {
  fx_deinit();
  layer_destroy(s_fx_layer);
  layer_destroy(s_digits_layer);
  layer_destroy(s_bg_layer);
}

static void prv_init(void) {
  settings_load();
  prv_apply_weather();
  digits_init();
  prv_update_time();
  prv_update_health();
  s_data.battery = battery_state_service_peek();

  s_window = window_create();
  window_set_background_color(s_window, theme_current()->background);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, false);

  app_message_register_inbox_received(prv_inbox_received);
  // Clay sends every setting in one dictionary, so keep the inbox roomy.
  app_message_open(512, 64);

  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick);
  battery_state_service_subscribe(prv_battery_handler);
  accel_tap_service_subscribe(prv_tap_handler);
  prv_apply_anim_settings();
#if defined(PBL_HEALTH)
  health_service_events_subscribe(prv_health_handler, NULL);
#endif
}

static void prv_deinit(void) {
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  accel_tap_service_unsubscribe();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
  digits_deinit();
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
