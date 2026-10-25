#include "settings.h"

#define SETTINGS_KEY 1
#define WEATHER_KEY 2

static Settings s_settings;
static WeatherCache s_weather;

static void prv_set_defaults(void) {
  s_settings = (Settings) {
    .theme = ThemeArcStorm,
    .top = { SlotWeather, SlotSunrise, SlotSunset },
    .bottom = { SlotHeartRate, SlotSteps, SlotBattery },
    .ring_outer = RingSteps,
    .ring_inner = RingBattery,
    .step_goal = 10000,
    .fahrenheit = true,
    .clock_format = ClockSystem,
    .anim_mode = AnimFull,
    .hr_pulse = PulseOnFlick,
    .wx_refresh_min = 30,
  };
}

void settings_load(void) {
  prv_set_defaults();
  // A blob of a different size was written by a different version of the
  // struct; reading it would scramble the fields, so fall back to defaults.
  if (persist_exists(SETTINGS_KEY)
      && persist_get_size(SETTINGS_KEY) == (int)sizeof(s_settings)) {
    persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
  }
  if (persist_exists(WEATHER_KEY)
      && persist_get_size(WEATHER_KEY) == (int)sizeof(s_weather)) {
    persist_read_data(WEATHER_KEY, &s_weather, sizeof(s_weather));
  } else {
    memset(&s_weather, 0, sizeof(s_weather));
  }
  theme_set(s_settings.theme);
}

void settings_save(void) {
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

Settings *settings_get(void) {
  return &s_settings;
}

WeatherCache *weather_cache_get(void) {
  return &s_weather;
}

void weather_cache_save(void) {
  persist_write_data(WEATHER_KEY, &s_weather, sizeof(s_weather));
}

bool settings_is_24h(void) {
  switch (s_settings.clock_format) {
    case Clock12h: return false;
    case Clock24h: return true;
    case ClockSystem:
    default: return clock_is_24h_style();
  }
}
