#include "rings.h"

// Each arc covers 120 degrees of the circle; the 60-degree wedges at the top
// and bottom stay empty so the slot rows have room.
#define ARC_RIGHT_START 30
#define ARC_RIGHT_END 150
#define ARC_LEFT_START 210
#define ARC_LEFT_END 330
#define ARC_SPAN (ARC_RIGHT_END - ARC_RIGHT_START)

#define TICK_STEP_DEG 15
#define TICK_LEN 4

static GRect prv_ring_rect(int radius) {
  return GRect(RING_CENTER_X - radius, RING_CENTER_Y - radius, radius * 2, radius * 2);
}

// Progress runs down the right arc first, then up the left arc, so the fill
// reads as one continuous sweep.
static int prv_progress_degrees(uint32_t value, uint32_t max) {
  if (max == 0) {
    return 0;
  }
  if (value > max) {
    value = max;
  }
  return (int)((value * (uint32_t)(2 * ARC_SPAN)) / max);
}

int32_t rings_progress_angle(uint32_t numerator, uint32_t denominator) {
  const int deg = prv_progress_degrees(numerator, denominator);
  const int abs_deg = (deg <= ARC_SPAN) ? (ARC_RIGHT_START + deg)
                                        : (ARC_LEFT_START + (deg - ARC_SPAN));
  return DEG_TO_TRIGANGLE(abs_deg);
}

static void prv_fill_arc(GContext *ctx, int radius, int thickness,
                         int start_deg, int end_deg) {
  if (end_deg <= start_deg) {
    return;
  }
  graphics_fill_radial(ctx, prv_ring_rect(radius), GOvalScaleModeFitCircle, thickness,
                       DEG_TO_TRIGANGLE(start_deg), DEG_TO_TRIGANGLE(end_deg));
}

static void prv_draw_ticks(GContext *ctx, int radius, int start_deg, int end_deg) {
  for (int deg = start_deg; deg <= end_deg; deg += TICK_STEP_DEG) {
    const int32_t angle = DEG_TO_TRIGANGLE(deg);
    const int32_t sin_a = sin_lookup(angle);
    const int32_t cos_a = cos_lookup(angle);
    const int outer = radius + 2;
    const int inner = radius + 2 - TICK_LEN;
    const GPoint a = {
      .x = RING_CENTER_X + (int16_t)(sin_a * outer / TRIG_MAX_RATIO),
      .y = RING_CENTER_Y - (int16_t)(cos_a * outer / TRIG_MAX_RATIO),
    };
    const GPoint b = {
      .x = RING_CENTER_X + (int16_t)(sin_a * inner / TRIG_MAX_RATIO),
      .y = RING_CENTER_Y - (int16_t)(cos_a * inner / TRIG_MAX_RATIO),
    };
    graphics_draw_line(ctx, a, b);
  }
}

void rings_draw(GContext *ctx, int radius, int thickness,
                uint32_t value, uint32_t max, GColor fill, GColor track,
                bool ticks) {
  graphics_context_set_fill_color(ctx, track);
  prv_fill_arc(ctx, radius, thickness, ARC_RIGHT_START, ARC_RIGHT_END);
  prv_fill_arc(ctx, radius, thickness, ARC_LEFT_START, ARC_LEFT_END);

  const int deg = prv_progress_degrees(value, max);
  graphics_context_set_fill_color(ctx, fill);
  prv_fill_arc(ctx, radius, thickness, ARC_RIGHT_START,
               ARC_RIGHT_START + (deg < ARC_SPAN ? deg : ARC_SPAN));
  if (deg > ARC_SPAN) {
    prv_fill_arc(ctx, radius, thickness, ARC_LEFT_START,
                 ARC_LEFT_START + (deg - ARC_SPAN));
  }

  if (ticks) {
    graphics_context_set_stroke_color(ctx, track);
    prv_draw_ticks(ctx, radius, ARC_RIGHT_START, ARC_RIGHT_END);
    prv_draw_ticks(ctx, radius, ARC_LEFT_START, ARC_LEFT_END);
  }
}
