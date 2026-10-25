# InkSeg: Watchy-Style E-Paper Watchface Spec for Claude Code

A stark, pure black-and-white Pebble watchface inspired by the classic Watchy (open-source e-paper watch) 7-segment face. Huge segment time, bold date, and a compact data block. No animation, no color: true 1-bit look.

Suggested working directory: `/home/chengmania/Nextcloud/projects/pebble/epaper`
Scaffold: `pebble new-project --c --javascript --ai epaper`

## Target
- Platform: **emery** (Pebble Time 2), 200x228
- Project type: watchface (`"watchface": true`)
- Only `GColorBlack` and `GColorWhite`. No grays, no dithering, no antialiasing on shapes where avoidable.
- Easy later port: `diorite` (Pebble 2, 1-bit) since the design is already 1-bit; would need layout scaling to 144x168.

## Look

```
┌──────────────────────┐
│                      │
│   ██ ██   ██ ██      │
│   ██ ██ : ██ ██      │  huge 7-segment time (no ghost segments)
│   ██ ██   ██ ██      │
│                      │
│  SUNDAY              │  day, bold
│  OCTOBER 25          │  month + date, bold
│  ━━━━━━━━━━━━━━━━━━   │  thick rule
│  👣 8,431     ▮▮▮▯    │  steps, battery
│  ♥ 72                │  heart rate
│  ☀ 58°F  ↑6:58 ↓6:12 │  weather, sunrise, sunset
└──────────────────────┘
```

### Design rules
- White background, black ink (Invert setting flips this).
- Left-aligned text with a consistent ~10 px left margin; strict vertical rhythm.
- Thick black rules (3 px) separate the time/date block from the data block.
- Time: drawn 7-segment digits with chunky segments and beveled ends, sized to fill the width (approx 44 px wide x 80 px tall per digit, tune to fit). Colon as two squares. Only lit segments drawn; no ghost segments (crisp e-paper look).
- 12h mode: small "AM"/"PM" text tucked top-right of the time block; no leading zero on hours (blank digit instead).
- Date: uppercase, bold system font (e.g. `FONT_KEY_GOTHIC_28_BOLD` or `LECO`); evaluate a custom pixel/bitmap font later if the system font looks too soft.
- Icons: small 1-bit pixel icons drawn in code or as tiny PNG resources (footprints, battery outline with fill bars, heart, weather set, sunrise/sunset arrows). Must be crisp at 1x.
- Battery icon: outline with 4 fill bars; show a small bolt when charging.

## Data
| Data | Source |
|---|---|
| Time | `localtime`, 12h/24h setting |
| Day / date | `strftime` (SUNDAY / OCTOBER 25) |
| Steps | `health_service_sum_today(HealthMetricStepCount)` |
| Heart rate | `health_service_peek_current_value(HealthMetricHeartRateBPM)`, `--` if unavailable |
| Battery | `battery_state_service` |
| Weather (temp + icon) | Phone JS, Open-Meteo, geolocation |
| Sunrise / sunset | Phone JS, Open-Meteo daily `sunrise`/`sunset` |

Show `--` for any value not yet available. Cache weather and sun times in persistent storage so they show instantly on launch.

## Settings page (Clay)
- **Invert:** Black on white (default) / White on black
- **Units:** °F / °C
- **Time format:** 12h / 24h
- **Leading zero (24h):** On / Off
- **Data rows:** toggle each of Steps, Battery, Heart rate, Weather, Sunrise/Sunset on or off (layout reflows to close gaps)
- **Weather refresh:** 15 / 30 / 60 min (default 30)
- Optional: **Date format:** "SUNDAY / OCTOBER 25" vs "SUN 25 OCT"

Persist with `persist_write_data`; apply without reload.

## Architecture

```
epaper/
├── package.json
├── src/
│   ├── c/
│   │   ├── main.c        (window, services, settings, AppMessage)
│   │   ├── seg.c/.h      (7-segment digit drawing)
│   │   └── icons.c/.h    (1-bit icon drawing)
│   └── pkjs/
│       ├── index.js      (weather, sunrise/sunset, Clay hookup)
│       └── config.js     (Clay settings)
```

### Watch side
- Minute `tick_timer_service`; single canvas layer redraw per minute (e-paper spirit: minimal updates).
- Health event subscription to refresh steps/HR; battery service for battery.
- Colors read from two variables (ink, paper) so Invert is a swap.

### Phone side
- Open-Meteo: `current=temperature_2m,weather_code`, `daily=sunrise,sunset`, `timezone=auto`, `forecast_days=1`, unit per setting.
- Map `weather_code` to icon IDs: clear, partly cloudy, cloudy, rain, snow, storm, fog.
- Send temp, icon ID, sunrise, sunset (minutes since midnight) via AppMessage on launch, on settings change, and on interval.

### package.json
- `"watchface": true`, `"targetPlatforms": ["emery"]`
- `"capabilities": ["configurable", "location", "health"]` (verify exact names for SDK 4.33.1)
- `messageKeys`: TEMP, WX_ICON, SUNRISE, SUNSET, UNIT, CLOCK_24H, LEADING_ZERO, INVERT, SHOW_STEPS, SHOW_BATTERY, SHOW_HR, SHOW_WX, SHOW_SUN, WX_REFRESH, DATE_FMT
- Dependency: `pebble-clay`

## Build order
1. Scaffold, convert to watchface, white screen on emery emulator.
2. 7-segment time, sized and positioned; 12h/24h, AM/PM.
3. Day/date block and thick rules.
4. 1-bit icons; steps, battery, heart rate rows.
5. Clay settings: invert, units, format, row toggles with reflow.
6. Phone-side weather and sunrise/sunset.
7. Polish: pixel-check icons and spacing with emulator screenshots, `--` placeholders, charging bolt.

## Testing
- `pebble build && pebble install --emulator emery`
- `pebble screenshot` to check pixel crispness
- Real watch via Developer Connection: `pebble install --phone <phone-ip>` (heart rate and steps best tested on the real watch)
