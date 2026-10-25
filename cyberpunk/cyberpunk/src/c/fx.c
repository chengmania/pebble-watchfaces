#include "fx.h"
#include "rings.h"

#define BOLT_MAX_POINTS 18
#define BOLT_BRANCH_POINTS 5
#define BOLT_STEP_Y 14
#define BOLT_JITTER 15

#define FLASH_FRAMES 2
#define BOLT_LAST_FRAME 14
#define GLITCH_LAST_FRAME 12

#define SPARK_TRAIL 5
#define CRACKLE_FRAMES 6
#define CRACKLE_ARCS 3
#define CRACKLE_PERIOD_MS 4000

#define BEAT_ATTACK_MS 150
// The pulse alone does not need the full frame rate: it only has to catch the
// moment the beat starts and the moment it finishes decaying.
#define PULSE_FRAME_MS 100
#define MIN_BPM 30
#define MAX_BPM 220

static Layer *s_bg_layer;
static Layer *s_digits_layer;
static Layer *s_fx_layer;

static AppTimer *s_frame_timer;
static AppTimer *s_crackle_timer;

static int s_strike_frame = -1;
static int s_spark_frame = -1;
static int s_crackle_frame = -1;

static uint32_t s_spark_value;
static uint32_t s_spark_max;

static bool s_pulse_always;
static bool s_charging;
static time_t s_pulse_until;

static GPoint s_bolt[BOLT_MAX_POINTS];
static int s_bolt_len;
static GPoint s_branch[BOLT_BRANCH_POINTS];
static int s_branch_len;

static int32_t s_crackle_angle[CRACKLE_ARCS];
static uint8_t s_last_phase;
static int s_last_bpm = 60;

static int prv_rand_range(int span) {
  return (rand() % (2 * span + 1)) - span;
}

bool fx_pulse_active(void) {
  return s_pulse_always || time(NULL) < s_pulse_until;
}

bool fx_active(void) {
  return s_strike_frame >= 0 || s_spark_frame >= 0 || s_crackle_frame >= 0;
}

// The frame timer runs whenever something on screen is still moving, and
// stops itself as soon as nothing is, so it costs nothing while idle.
static void prv_timer_callback(void *context);

static void prv_ensure_timer(void) {
  if (s_frame_timer) {
    return;
  }
  if (fx_active()) {
    s_frame_timer = app_timer_register(FX_FRAME_MS, prv_timer_callback, NULL);
  } else if (fx_pulse_active()) {
    s_frame_timer = app_timer_register(PULSE_FRAME_MS, prv_timer_callback, NULL);
  }
}

static void prv_timer_callback(void *context) {
  s_frame_timer = NULL;

  const bool strike_was_on = s_strike_frame >= 0;
  const bool overlay_was_on = fx_active();

  if (s_strike_frame >= 0 && ++s_strike_frame >= FX_FRAMES) {
    s_strike_frame = -1;
  }
  if (s_spark_frame >= 0 && ++s_spark_frame >= FX_SPARK_FRAMES) {
    s_spark_frame = -1;
  }
  if (s_crackle_frame >= 0 && ++s_crackle_frame >= CRACKLE_FRAMES) {
    s_crackle_frame = -1;
  }

  // Only repaint what actually changed. In particular a standing heart pulse
  // must not drag the digits and overlay through a redraw 30 times a second.
  if (strike_was_on && s_digits_layer) {
    layer_mark_dirty(s_digits_layer);
  }
  if (overlay_was_on && s_fx_layer) {
    layer_mark_dirty(s_fx_layer);
  }
  if (s_bg_layer) {
    const uint8_t phase = fx_heart_phase(s_last_bpm);
    if (phase != s_last_phase) {
      s_last_phase = phase;
      layer_mark_dirty(s_bg_layer);
    }
  }

  prv_ensure_timer();
}

// Builds a fresh jagged polyline from the top of the screen to the bottom,
// with one branch peeling off part way down.
static void prv_generate_bolt(GRect bounds) {
  int x = bounds.size.w / 2 + prv_rand_range(bounds.size.w / 3);
  int y = 0;

  s_bolt_len = 0;
  while (y < bounds.size.h && s_bolt_len < BOLT_MAX_POINTS) {
    s_bolt[s_bolt_len++] = GPoint(x, y);
    x += prv_rand_range(BOLT_JITTER);
    if (x < 4) { x = 4; }
    if (x > bounds.size.w - 4) { x = bounds.size.w - 4; }
    y += BOLT_STEP_Y / 2 + (rand() % BOLT_STEP_Y);
  }

  // Branch out of a point in the middle third of the bolt.
  s_branch_len = 0;
  if (s_bolt_len > 6) {
    const int from = s_bolt_len / 3 + (rand() % (s_bolt_len / 3));
    const int dir = (rand() % 2) ? 1 : -1;
    int bx = s_bolt[from].x;
    int by = s_bolt[from].y;
    while (s_branch_len < BOLT_BRANCH_POINTS && by < bounds.size.h) {
      s_branch[s_branch_len++] = GPoint(bx, by);
      bx += dir * (6 + (rand() % 12));
      by += 4 + (rand() % 10);
      if (bx < 2 || bx > bounds.size.w - 2) { break; }
    }
  }
}

static void prv_crackle_timer(void *context) {
  s_crackle_timer = NULL;
  if (!s_charging) {
    return;
  }
  for (int i = 0; i < CRACKLE_ARCS; i++) {
    s_crackle_angle[i] = DEG_TO_TRIGANGLE(rand() % 360);
  }
  s_crackle_frame = 0;
  prv_ensure_timer();
  s_crackle_timer = app_timer_register(CRACKLE_PERIOD_MS, prv_crackle_timer, NULL);
}

void fx_init(Layer *bg_layer, Layer *digits_layer, Layer *fx_layer) {
  s_bg_layer = bg_layer;
  s_digits_layer = digits_layer;
  s_fx_layer = fx_layer;
  s_strike_frame = s_spark_frame = s_crackle_frame = -1;
}

void fx_deinit(void) {
  if (s_frame_timer) {
    app_timer_cancel(s_frame_timer);
    s_frame_timer = NULL;
  }
  if (s_crackle_timer) {
    app_timer_cancel(s_crackle_timer);
    s_crackle_timer = NULL;
  }
  s_strike_frame = s_spark_frame = s_crackle_frame = -1;
  s_bg_layer = s_digits_layer = s_fx_layer = NULL;
}

void fx_cancel_all(void) {
  s_strike_frame = s_spark_frame = s_crackle_frame = -1;
  s_pulse_until = 0;
  s_pulse_always = false;
  if (s_frame_timer) {
    app_timer_cancel(s_frame_timer);
    s_frame_timer = NULL;
  }
  if (s_crackle_timer) {
    app_timer_cancel(s_crackle_timer);
    s_crackle_timer = NULL;
  }
  s_charging = false;
  s_last_phase = 0;
  if (s_bg_layer) { layer_mark_dirty(s_bg_layer); }
  if (s_digits_layer) { layer_mark_dirty(s_digits_layer); }
  if (s_fx_layer) { layer_mark_dirty(s_fx_layer); }
}

void fx_strike(void) {
  if (s_strike_frame >= 0 || !s_fx_layer) {
    return;
  }
  prv_generate_bolt(layer_get_bounds(s_fx_layer));
  s_strike_frame = 0;
  s_pulse_until = time(NULL) + FX_PULSE_WINDOW_S;
  layer_mark_dirty(s_fx_layer);
  if (s_digits_layer) { layer_mark_dirty(s_digits_layer); }
  prv_ensure_timer();
}

void fx_spark(uint32_t value, uint32_t max) {
  if (s_spark_frame >= 0 || max == 0) {
    return;
  }
  s_spark_value = value;
  s_spark_max = max;
  s_spark_frame = 0;
  prv_ensure_timer();
}

void fx_set_charging(bool charging) {
  if (charging == s_charging) {
    return;
  }
  s_charging = charging;
  if (charging) {
    if (!s_crackle_timer) {
      s_crackle_timer = app_timer_register(CRACKLE_PERIOD_MS, prv_crackle_timer, NULL);
    }
  } else if (s_crackle_timer) {
    app_timer_cancel(s_crackle_timer);
    s_crackle_timer = NULL;
  }
}

void fx_set_pulse_always(bool always) {
  s_pulse_always = always;
  prv_ensure_timer();
}

uint8_t fx_heart_phase(int bpm) {
  // Clamp before dividing: a bogus reading of 0 or a huge one would otherwise
  // give a beat period of zero and a modulo by zero below.
  if (bpm < MIN_BPM || bpm > MAX_BPM) {
    bpm = 60;
  } else {
    s_last_bpm = bpm;
  }
  if (!fx_pulse_active()) {
    return 0;
  }
  time_t s;
  uint16_t ms;
  time_ms(&s, &ms);
  const uint32_t period = 60000 / (uint32_t)bpm;
  const uint32_t t = (uint32_t)(((uint64_t)s * 1000 + ms) % period);
  if (t >= BEAT_ATTACK_MS) {
    return 0;
  }
  // Snap bright on the beat, then fall away.
  return (uint8_t)(255 - (t * 255 / BEAT_ATTACK_MS));
}

static void prv_draw_polyline(GContext *ctx, const GPoint *pts, int len) {
  for (int i = 1; i < len; i++) {
    graphics_draw_line(ctx, pts[i - 1], pts[i]);
  }
}

static GPoint prv_ring_point(int32_t angle, int radius) {
  return (GPoint) {
    .x = RING_CENTER_X + (int16_t)(sin_lookup(angle) * radius / TRIG_MAX_RATIO),
    .y = RING_CENTER_Y - (int16_t)(cos_lookup(angle) * radius / TRIG_MAX_RATIO),
  };
}

static void prv_render_strike(GContext *ctx, GRect bounds, const ArcStormTheme *theme) {
  // A couple of frames of bright background, as the strike lands.
  if (s_strike_frame < FLASH_FRAMES) {
    graphics_context_set_fill_color(ctx,
        s_strike_frame == 0 ? theme->highlight : theme->primary);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  }
  if (s_strike_frame > BOLT_LAST_FRAME) {
    return;
  }

  // Wide cyan body first, then a one-pixel white core on top.
  const bool fading = s_strike_frame > BOLT_LAST_FRAME - 4;
  graphics_context_set_stroke_width(ctx, fading ? 1 : 3);
  graphics_context_set_stroke_color(ctx, fading ? theme->primary_dim : theme->primary);
  prv_draw_polyline(ctx, s_bolt, s_bolt_len);
  prv_draw_polyline(ctx, s_branch, s_branch_len);

  if (!fading) {
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_stroke_color(ctx, theme->highlight);
    prv_draw_polyline(ctx, s_bolt, s_bolt_len);
    prv_draw_polyline(ctx, s_branch, s_branch_len);
  }
  graphics_context_set_stroke_width(ctx, 1);
}

// A bright dot runs the outer ring from zero to the current progress,
// with a short trail fading out behind it.
static void prv_render_spark(GContext *ctx, const ArcStormTheme *theme) {
  const int radius = RING_OUTER_R - RING_OUTER_T / 2;
  for (int t = SPARK_TRAIL; t >= 0; t--) {
    const int frame = s_spark_frame - t;
    if (frame < 0) {
      continue;
    }
    const uint32_t travelled = s_spark_value * (uint32_t)frame / (FX_SPARK_FRAMES - 1);
    const GPoint p = prv_ring_point(rings_progress_angle(travelled, s_spark_max), radius);
    graphics_context_set_fill_color(ctx, (t == 0) ? theme->highlight : theme->primary);
    graphics_fill_circle(ctx, p, (t == 0) ? 3 : (t < 3 ? 2 : 1));
  }
}

// Short sparks jumping off the battery ring while the watch is charging.
static void prv_render_crackle(GContext *ctx, const ArcStormTheme *theme) {
  graphics_context_set_stroke_color(ctx,
      (s_crackle_frame % 2) ? theme->highlight : theme->primary);
  for (int i = 0; i < CRACKLE_ARCS; i++) {
    const int32_t angle = s_crackle_angle[i] + DEG_TO_TRIGANGLE(s_crackle_frame * 3);
    const GPoint a = prv_ring_point(angle, RING_INNER_R - 3);
    const GPoint b = prv_ring_point(angle + DEG_TO_TRIGANGLE(4), RING_INNER_R + 4);
    const GPoint c = prv_ring_point(angle + DEG_TO_TRIGANGLE(8), RING_INNER_R - 1);
    graphics_draw_line(ctx, a, b);
    graphics_draw_line(ctx, b, c);
  }
}

void fx_render(GContext *ctx, GRect bounds, const ArcStormTheme *theme) {
  if (s_spark_frame >= 0) {
    prv_render_spark(ctx, theme);
  }
  if (s_crackle_frame >= 0) {
    prv_render_crackle(ctx, theme);
  }
  if (s_strike_frame >= 0) {
    prv_render_strike(ctx, bounds, theme);
  }
}

void fx_fill_glitch(DigitsGlitch *out) {
  memset(out, 0, sizeof(*out));
  if (s_strike_frame < 0 || s_strike_frame > GLITCH_LAST_FRAME) {
    return;
  }
  // Shove each digit sideways and tear the three segment bands apart, so the
  // time looks like a signal breaking up.
  for (int i = 0; i < 4; i++) {
    out->slice_offset[i] = (int8_t)prv_rand_range(3);
  }
  for (int i = 0; i < 3; i++) {
    out->row_offset[i] = (int8_t)prv_rand_range(4);
  }
  out->swap_colors = (s_strike_frame % 2) == 1;
}
