#pragma once

#include <pebble.h>

#include "slots.h"
#include "theme.h"

typedef enum {
  RingSteps = 0,
  RingBattery,
  RingOff,
  RingSourceCount,
} RingSource;

typedef enum {
  ClockSystem = 0,
  Clock12h,
  Clock24h,
  ClockFormatCount,
} ClockFormat;

typedef enum {
  AnimFull = 0,
  AnimCalm,
  AnimModeCount,
} AnimMode;

typedef enum {
  PulseOnFlick = 0,
  PulseAlways,
  HeartPulseModeCount,
} HeartPulseMode;

typedef struct {
  ThemeId theme;
  SlotType top[3];
  SlotType bottom[3];
  RingSource ring_outer;
  RingSource ring_inner;
  uint32_t step_goal;
  bool fahrenheit;
  ClockFormat clock_format;
  AnimMode anim_mode;
  HeartPulseMode hr_pulse;
  uint8_t wx_refresh_min;
} Settings;

// Cached weather, kept in persistent storage so it shows instantly on launch.
typedef struct {
  bool valid;
  int temp;
  uint8_t wx_icon;
  int16_t sunrise_min;
  int16_t sunset_min;
  bool have_sun;
  time_t updated;        // when the phone last sent an update
} WeatherCache;

// A cached reading is shown on launch, but not forever: stale values are
// worse than an honest "--".
#define WEATHER_MAX_AGE_S (6 * 60 * 60)
#define SUN_MAX_AGE_S (24 * 60 * 60)

void settings_load(void);
void settings_save(void);
Settings *settings_get(void);
WeatherCache *weather_cache_get(void);
void weather_cache_save(void);

// True when the watch should render a 24-hour clock.
bool settings_is_24h(void);
