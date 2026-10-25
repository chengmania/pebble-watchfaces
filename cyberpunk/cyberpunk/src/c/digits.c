#include "digits.h"

// Segment bit assignments for a 7-segment layout.
#define SEG_A (1 << 0)  // top
#define SEG_B (1 << 1)  // top right
#define SEG_C (1 << 2)  // bottom right
#define SEG_D (1 << 3)  // bottom
#define SEG_E (1 << 4)  // bottom left
#define SEG_F (1 << 5)  // top left
#define SEG_G (1 << 6)  // middle

static const uint8_t s_digit_segments[10] = {
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,          // 0
  SEG_B | SEG_C,                                          // 1
  SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,                  // 2
  SEG_A | SEG_B | SEG_G | SEG_C | SEG_D,                  // 3
  SEG_F | SEG_G | SEG_B | SEG_C,                          // 4
  SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,                  // 5
  SEG_A | SEG_F | SEG_G | SEG_E | SEG_C | SEG_D,          // 6
  SEG_A | SEG_B | SEG_C,                                  // 7
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,  // 8
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,          // 9
};

// Half-height of a vertical segment, measured from the digit edge to the
// centre line, including the middle bar so the corners interlock.
#define SEG_V_H ((DIGIT_H + DIGIT_T) / 2)
#define SEG_H_W DIGIT_W
#define HALF_T (DIGIT_T / 2)

// Beveled hexagons: pointed ends give the segments their angular, chamfered
// look instead of the blunt ends of a stock LCD font.
static GPoint s_h_points[6];
static GPoint s_v_points[6];
static GPathInfo s_h_info = { .num_points = 6, .points = s_h_points };
static GPathInfo s_v_info = { .num_points = 6, .points = s_v_points };
static GPath *s_h_path;
static GPath *s_v_path;

void digits_init(void) {
  s_h_points[0] = GPoint(HALF_T, 0);
  s_h_points[1] = GPoint(SEG_H_W - HALF_T, 0);
  s_h_points[2] = GPoint(SEG_H_W, HALF_T);
  s_h_points[3] = GPoint(SEG_H_W - HALF_T, DIGIT_T);
  s_h_points[4] = GPoint(HALF_T, DIGIT_T);
  s_h_points[5] = GPoint(0, HALF_T);

  s_v_points[0] = GPoint(0, HALF_T);
  s_v_points[1] = GPoint(HALF_T, 0);
  s_v_points[2] = GPoint(DIGIT_T, HALF_T);
  s_v_points[3] = GPoint(DIGIT_T, SEG_V_H - HALF_T);
  s_v_points[4] = GPoint(HALF_T, SEG_V_H);
  s_v_points[5] = GPoint(0, SEG_V_H - HALF_T);

  s_h_path = gpath_create(&s_h_info);
  s_v_path = gpath_create(&s_v_info);
}

void digits_deinit(void) {
  gpath_destroy(s_h_path);
  gpath_destroy(s_v_path);
  s_h_path = NULL;
  s_v_path = NULL;
}

static void prv_draw_h(GContext *ctx, int x, int y) {
  gpath_move_to(s_h_path, GPoint(x, y));
  gpath_draw_filled(ctx, s_h_path);
}

static void prv_draw_v(GContext *ctx, int x, int y) {
  gpath_move_to(s_v_path, GPoint(x, y));
  gpath_draw_filled(ctx, s_v_path);
}

// Draws one digit with its top-left corner at (x, y) in the current fill
// color. `rows` shifts the top, middle and bottom bands independently, which
// is what gives the glitch its torn-scanline look; pass NULL for a clean draw.
static void prv_draw_digit(GContext *ctx, int x, int y, int value,
                           const int8_t *rows) {
  if (value < 0 || value > 9) {
    return;
  }
  const uint8_t segs = s_digit_segments[value];
  const int mid_y = (DIGIT_H - DIGIT_T) / 2;
  const int top = x + (rows ? rows[0] : 0);
  const int mid = x + (rows ? rows[1] : 0);
  const int bot = x + (rows ? rows[2] : 0);
  const int lower_y = y + DIGIT_H / 2 - HALF_T;
  const int right = DIGIT_W - DIGIT_T;

  if (segs & SEG_A) { prv_draw_h(ctx, top, y); }
  if (segs & SEG_G) { prv_draw_h(ctx, mid, y + mid_y); }
  if (segs & SEG_D) { prv_draw_h(ctx, bot, y + DIGIT_H - DIGIT_T); }
  if (segs & SEG_F) { prv_draw_v(ctx, top, y); }
  if (segs & SEG_B) { prv_draw_v(ctx, top + right, y); }
  if (segs & SEG_E) { prv_draw_v(ctx, bot, lower_y); }
  if (segs & SEG_C) { prv_draw_v(ctx, bot + right, lower_y); }
}

// The colon is two beveled squares on the digit centre line.
static void prv_draw_colon(GContext *ctx, int x, int y) {
  const int size = DIGIT_T;
  const int top = y + DIGIT_H / 4 - size / 2;
  const int bottom = y + (DIGIT_H * 3) / 4 - size / 2;
  const int cx = x + (COLON_W - size) / 2;
  graphics_fill_rect(ctx, GRect(cx, top, size, size), 2, GCornersAll);
  graphics_fill_rect(ctx, GRect(cx, bottom, size, size), 2, GCornersAll);
}

// x offsets of the four digits and the colon, relative to the block origin.
static void prv_slot_offsets(int *out) {
  out[0] = 0;
  out[1] = DIGIT_W + DIGIT_GAP;
  out[4] = out[1] + DIGIT_W + DIGIT_GAP + 1;          // colon
  out[2] = out[4] + COLON_W + DIGIT_GAP + 1;
  out[3] = out[2] + DIGIT_W + DIGIT_GAP;
}

// Leftmost lit column within a digit cell: only "1" is inset, because it
// lights the two right-hand segments and nothing else.
static int prv_digit_ink_left(int value) {
  return (value == 1) ? (DIGIT_W - DIGIT_T) : 0;
}

static int prv_time_digits(int hour, int minute, bool hide_leading_zero, int *out) {
  out[0] = (hide_leading_zero && hour < 10) ? -1 : hour / 10;
  out[1] = hour % 10;
  out[2] = minute / 10;
  out[3] = minute % 10;
  return 4;
}

void digits_time_ink_bounds(int hour, int minute, bool hide_leading_zero,
                            int *left, int *right) {
  int digits[4];
  int offsets[5];
  prv_time_digits(hour, minute, hide_leading_zero, digits);
  prv_slot_offsets(offsets);

  // The colon is always lit, so start from its extent.
  int lo = offsets[4] + (COLON_W - DIGIT_T) / 2;
  int hi = lo + DIGIT_T;

  for (int i = 0; i < 4; i++) {
    if (digits[i] < 0) {
      continue;
    }
    const int l = offsets[i] + prv_digit_ink_left(digits[i]);
    const int r = offsets[i] + DIGIT_W;
    if (l < lo) { lo = l; }
    if (r > hi) { hi = r; }
  }

  // The highlight pass sits 1 px up-left and the shadow 2 px down-right.
  *left = lo - 1;
  *right = hi + 2;
}

static void prv_draw_pass(GContext *ctx, GPoint origin, const int *digits,
                          const int *offsets, int dx, int dy,
                          const DigitsGlitch *glitch) {
  for (int i = 0; i < 4; i++) {
    const int slice = glitch ? glitch->slice_offset[i] : 0;
    prv_draw_digit(ctx, origin.x + offsets[i] + dx + slice, origin.y + dy, digits[i],
                   glitch ? glitch->row_offset : NULL);
  }
  prv_draw_colon(ctx, origin.x + offsets[4] + dx, origin.y + dy);
}

void digits_draw_time(GContext *ctx, GPoint origin, int hour, int minute,
                      bool hide_leading_zero, const ArcStormTheme *theme,
                      const DigitsGlitch *glitch) {
  int digits[4];
  prv_time_digits(hour, minute, hide_leading_zero, digits);
  int offsets[5];
  prv_slot_offsets(offsets);

  const bool swapped = glitch && glitch->swap_colors;

  // Drop shadow, offset down-right.
  graphics_context_set_fill_color(ctx, theme->shadow);
  prv_draw_pass(ctx, origin, digits, offsets, 2, 2, glitch);

  // White rim, offset up-left, left exposed as a 1 px inner highlight.
  graphics_context_set_fill_color(ctx, swapped ? theme->warning : theme->highlight);
  prv_draw_pass(ctx, origin, digits, offsets, -1, -1, glitch);

  // Main face.
  graphics_context_set_fill_color(ctx, swapped ? theme->highlight : theme->primary);
  prv_draw_pass(ctx, origin, digits, offsets, 0, 0, glitch);
}
