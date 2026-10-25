#pragma once

#include <pebble.h>

#include "digits.h"
#include "theme.h"

// One strike runs for FX_FRAMES frames at FX_FRAME_MS, so just under a second.
#define FX_FRAME_MS 33
#define FX_FRAMES 30
#define FX_SPARK_FRAMES 15
#define FX_PULSE_WINDOW_S 10

void fx_init(Layer *bg_layer, Layer *digits_layer, Layer *fx_layer);
void fx_deinit(void);

// Starts a lightning strike. Repeat calls while one is running are ignored,
// so a burst of taps cannot stack animations.
void fx_strike(void);

// Runs the spark along the outer ring, up to `value`/`max` of the way round.
void fx_spark(uint32_t value, uint32_t max);

// While charging, a short crackle plays along the battery ring periodically.
void fx_set_charging(bool charging);

// "Always" keeps the heart beating; otherwise it only beats for
// FX_PULSE_WINDOW_S after a strike.
void fx_set_pulse_always(bool always);
bool fx_pulse_active(void);

// 0 at rest, 255 at the peak of a beat at the given BPM.
uint8_t fx_heart_phase(int bpm);

bool fx_active(void);

// Stops every running effect immediately; used when Calm mode is switched on.
void fx_cancel_all(void);

// Draws the flash, bolt, spark and crackle for the current frame.
void fx_render(GContext *ctx, GRect bounds, const ArcStormTheme *theme);

// Fills in the digit glitch for the current frame; zeroed when idle.
void fx_fill_glitch(DigitsGlitch *out);
