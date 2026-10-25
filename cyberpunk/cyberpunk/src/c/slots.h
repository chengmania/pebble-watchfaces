#pragma once

#include <pebble.h>

#include "theme.h"

typedef enum {
  SlotOff = 0,
  SlotWeather,
  SlotSunrise,
  SlotSunset,
  SlotSunriseSunset,
  SlotHeartRate,
  SlotSteps,
  SlotBattery,
  SlotDate,
  SlotTypeCount,
} SlotType;

// Weather icon ids, as mapped from the Open-Meteo weather code phone side.
typedef enum {
  WxClear = 0,
  WxCloudy,
  WxRain,
  WxSnow,
  WxStorm,
  WxFog,
  WxIconCount,
} WeatherIcon;

typedef struct {
  bool have_weather;
  int temp;              // already in the configured unit
  WeatherIcon wx_icon;

  bool have_sun;
  int sunrise_min;       // minutes since local midnight
  int sunset_min;

  bool have_hr;
  int hr;

  uint32_t steps;
  BatteryChargeState battery;

  const char *date;        // full form, e.g. SUN OCT 25
  const char *date_short;  // slot-sized form, e.g. OCT 25
  bool clock_24h;

  // 0 = resting, 255 = full beat; drives the heart icon pulse.
  uint8_t heart_phase;
} SlotData;

#define SLOT_ROW_H 26

void slots_draw(GContext *ctx, GRect rect, SlotType type, GTextAlignment align,
                const SlotData *data, const ArcStormTheme *theme);
