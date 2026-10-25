#include "seg.h"

// Length of a drawn horizontal segment (trimmed at both ends).
#define SEG_HLEN (SEG_W - 2 * SEG_GAP)
// Full span of a vertical segment, half the digit plus one thickness.
#define SEG_VSPAN ((SEG_H + SEG_T) / 2)
#define SEG_VLEN (SEG_VSPAN - 2 * SEG_GAP)
// Top of the middle (G) segment.
#define SEG_MIDY ((SEG_H - SEG_T) / 2)

#define S_A 0x01
#define S_B 0x02
#define S_C 0x04
#define S_D 0x08
#define S_E 0x10
#define S_F 0x20
#define S_G 0x40

static const uint8_t s_masks[10] = {
  S_A | S_B | S_C | S_D | S_E | S_F,        // 0
  S_B | S_C,                                // 1
  S_A | S_B | S_G | S_E | S_D,              // 2
  S_A | S_B | S_G | S_C | S_D,              // 3
  S_F | S_G | S_B | S_C,                    // 4
  S_A | S_F | S_G | S_C | S_D,              // 5
  S_A | S_F | S_G | S_E | S_C | S_D,        // 6
  S_A | S_B | S_C,                          // 7
  S_A | S_B | S_C | S_D | S_E | S_F | S_G,  // 8
  S_A | S_B | S_C | S_D | S_F | S_G,        // 9
};

// Beveled horizontal segment, drawn in local coordinates.
static GPoint s_h_pts[6] = {
  {0,                   SEG_T / 2},
  {SEG_T / 2,           0},
  {SEG_HLEN - SEG_T / 2, 0},
  {SEG_HLEN,            SEG_T / 2},
  {SEG_HLEN - SEG_T / 2, SEG_T},
  {SEG_T / 2,           SEG_T},
};
static GPathInfo s_h_info = { 6, s_h_pts };

// Beveled vertical segment.
static GPoint s_v_pts[6] = {
  {SEG_T / 2, 0},
  {SEG_T,     SEG_T / 2},
  {SEG_T,     SEG_VLEN - SEG_T / 2},
  {SEG_T / 2, SEG_VLEN},
  {0,         SEG_VLEN - SEG_T / 2},
  {0,         SEG_T / 2},
};
static GPathInfo s_v_info = { 6, s_v_pts };

static GPath *s_h_path;
static GPath *s_v_path;

void seg_init(void) {
  s_h_path = gpath_create(&s_h_info);
  s_v_path = gpath_create(&s_v_info);
}

void seg_deinit(void) {
  gpath_destroy(s_h_path);
  gpath_destroy(s_v_path);
}

static void prv_h(GContext *ctx, int x, int y) {
  gpath_move_to(s_h_path, GPoint(x, y));
  gpath_draw_filled(ctx, s_h_path);
}

static void prv_v(GContext *ctx, int x, int y) {
  gpath_move_to(s_v_path, GPoint(x, y));
  gpath_draw_filled(ctx, s_v_path);
}

void seg_draw_digit(GContext *ctx, GPoint o, int digit) {
  if (digit < 0 || digit > 9) {
    return;  // blank: draw nothing, no ghost segments
  }
  const uint8_t m = s_masks[digit];
  const int x = o.x, y = o.y;

  if (m & S_A) prv_h(ctx, x + SEG_GAP, y);
  if (m & S_G) prv_h(ctx, x + SEG_GAP, y + SEG_MIDY);
  if (m & S_D) prv_h(ctx, x + SEG_GAP, y + SEG_H - SEG_T);
  if (m & S_F) prv_v(ctx, x, y + SEG_GAP);
  if (m & S_B) prv_v(ctx, x + SEG_W - SEG_T, y + SEG_GAP);
  if (m & S_E) prv_v(ctx, x, y + SEG_MIDY + SEG_GAP);
  if (m & S_C) prv_v(ctx, x + SEG_W - SEG_T, y + SEG_MIDY + SEG_GAP);
}

void seg_draw_colon(GContext *ctx, GPoint o) {
  // Squares sit symmetrically at the thirds of the digit height.
  const int y1 = SEG_H / 3 - SEG_T / 2;
  const int y2 = SEG_H - SEG_H / 3 - SEG_T / 2;
  graphics_fill_rect(ctx, GRect(o.x, o.y + y1, SEG_COLON_W, SEG_COLON_W), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(o.x, o.y + y2, SEG_COLON_W, SEG_COLON_W), 0, GCornerNone);
}
