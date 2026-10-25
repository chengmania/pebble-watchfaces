#pragma once
#include <pebble.h>

// A crisp 1-bit icon: `h` rows of up to 16 pixels, MSB = leftmost pixel.
typedef struct {
  uint8_t w;
  uint8_t h;
  const uint16_t *rows;
} IconBitmap;

extern const IconBitmap ICON_STEPS;
extern const IconBitmap ICON_HEART;
extern const IconBitmap ICON_BOLT;
extern const IconBitmap ICON_BT_OFF;
extern const IconBitmap ICON_ARROW_UP;
extern const IconBitmap ICON_ARROW_DOWN;

// Weather icons, indexed by WxIcon.
extern const IconBitmap ICON_WX_CLEAR;
extern const IconBitmap ICON_WX_PARTLY;
extern const IconBitmap ICON_WX_CLOUDY;
extern const IconBitmap ICON_WX_RAIN;
extern const IconBitmap ICON_WX_SNOW;
extern const IconBitmap ICON_WX_STORM;
extern const IconBitmap ICON_WX_FOG;

typedef enum {
  WxClear = 0,
  WxPartly,
  WxCloudy,
  WxRain,
  WxSnow,
  WxStorm,
  WxFog,
  WxCount,
} WxIcon;

const IconBitmap *icon_for_weather(int wx_icon);

// Draws the icon in the context's current fill colour.
void icon_draw(GContext *ctx, const IconBitmap *icon, GPoint at);

// Battery outline with 4 fill bars; `bars` is 0-4. Occupies BATT_W x BATT_H.
#define BATT_W 26
#define BATT_H 12
void icon_draw_battery(GContext *ctx, GPoint at, int bars, bool charging);
