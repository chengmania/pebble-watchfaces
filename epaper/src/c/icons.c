#include "icons.h"

#include "icons_data.inc"

static const IconBitmap *const s_wx[WxCount] = {
  &ICON_WX_CLEAR, &ICON_WX_PARTLY, &ICON_WX_CLOUDY, &ICON_WX_RAIN,
  &ICON_WX_SNOW, &ICON_WX_STORM, &ICON_WX_FOG,
};

const IconBitmap *icon_for_weather(int wx_icon) {
  if (wx_icon < 0 || wx_icon >= WxCount) return &ICON_WX_CLOUDY;
  return s_wx[wx_icon];
}

void icon_draw(GContext *ctx, const IconBitmap *icon, GPoint at) {
  for (int y = 0; y < icon->h; y++) {
    uint16_t row = icon->rows[y];
    int x = 0;
    while (x < icon->w) {
      if (row & (0x8000 >> x)) {
        int run = 0;
        while (x + run < icon->w && (row & (0x8000 >> (x + run)))) {
          run++;
        }
        graphics_fill_rect(ctx, GRect(at.x + x, at.y + y, run, 1), 0, GCornerNone);
        x += run;
      } else {
        x++;
      }
    }
  }
}

void icon_draw_battery(GContext *ctx, GPoint at, int bars, bool charging) {
  const int x = at.x, y = at.y;
  const int bw = BATT_W - 2;  // body width; the nub takes the last 2px

  // 2px outline drawn as four bars, so the interior stays paper-coloured.
  graphics_fill_rect(ctx, GRect(x, y, bw, 2), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x, y + BATT_H - 2, bw, 2), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x, y, 2, BATT_H), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + bw - 2, y, 2, BATT_H), 0, GCornerNone);
  // Positive terminal nub.
  graphics_fill_rect(ctx, GRect(x + bw, y + 4, 2, 4), 0, GCornerNone);

  if (bars > 4) bars = 4;
  for (int i = 0; i < bars; i++) {
    graphics_fill_rect(ctx, GRect(x + 3 + i * 5, y + 3, 3, BATT_H - 6), 0, GCornerNone);
  }

  if (charging) {
    icon_draw(ctx, &ICON_BOLT, GPoint(x - ICON_BOLT.w - 3, y + (BATT_H - ICON_BOLT.h) / 2));
  }
}
