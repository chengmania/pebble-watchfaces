# LCD Watchface: Project Spec for Claude Code

A minimalist Pebble watchface with a big 7-segment LCD time (ghosted "off" segments), a second world clock, and four swappable complication slots. Settings are configured through a Clay page in the Pebble phone app.

## Target
- Platform: **emery** (Pebble Time 2), 200x228, color (64-color palette)
- Project type: watchface (`"watchface": true`)
- Optional later: add `basalt` / `diorite` / `chalk` targets (needs layout changes)

## Look
Inspired by a retro LCD watch screenshot: framed panels, chunky 7-segment digits, ghosted unlit segments (dim, like a real LCD).

```
┌──────────────────────┐
│  SLOT A      SLOT B  │   e.g. TEMP 72F / STEPS 8,431
├──────────────────────┤
│                      │
│   big LCD time    PM │   main time, 7-segment, AM/PM marker
│                      │
├──────────────────────┤
│  LON  03:42 AM       │   second clock (UTC or a city), smaller 7-segment
├──────────────────────┤
│  SLOT C      SLOT D  │   e.g. DAY SAT / DATE OCT 24
└──────────────────────┘
```

Minimal: no year, no Bluetooth icon, no battery bar by default (Battery is available as a slot option).

## Themes
Defined as a struct of colors so more can be added easily.

| Theme | Frame/background | Panel | Lit digits | Ghost segments |
|---|---|---|---|---|
| Classic Grey | Black | Light grey | Black | Dim grey (dotted/dithered if feasible) |
| Retro Amber | Black | Black | Bright amber | Dim brown |

## Complication slots
Four slots (A, B, C, D). Each can be set to one of:
**Temp, Steps, Date, Day, Battery, Off**

Defaults: A = Temp, B = Steps, C = Day, D = Date.

## Settings page (Clay)
- **Theme:** Classic Grey / Retro Amber
- **Slots A to D:** dropdown each (options above)
- **Second clock:** UTC (pinned first) plus 30 cities
- **Units:** Temp °F / °C
- **Time format:** 12h / 24h

### Second clock city list (IANA zones)
UTC, then:
- Americas: Pacific/Honolulu, America/Anchorage, America/Los_Angeles, America/Denver, America/Chicago, America/New_York, America/Toronto, America/Mexico_City, America/Sao_Paulo, America/Argentina/Buenos_Aires
- Europe/Africa: Europe/London, Europe/Paris, Europe/Berlin, Europe/Rome, Europe/Istanbul, Europe/Moscow, Africa/Cairo, Africa/Lagos, Africa/Nairobi, Africa/Johannesburg
- Asia/Pacific: Asia/Dubai, Asia/Kolkata, Asia/Bangkok, Asia/Singapore, Asia/Hong_Kong, Asia/Shanghai, Asia/Tokyo, Asia/Seoul, Australia/Sydney, Pacific/Auckland

Short labels shown on watch: UTC, HNL, ANC, LAX, DEN, CHI, NYC, TOR, MEX, SAO, BUE, LON, PAR, BER, ROM, IST, MOS, CAI, LOS, NBO, JNB, DXB, BOM, BKK, SIN, HKG, SHA, TYO, SEL, SYD, AKL.

## Architecture

```
lcd-watch/
├── package.json         (metadata, emery target, messageKeys, capabilities)
├── src/
│   ├── c/main.c         (drawing, segments, complications, settings storage)
│   └── pkjs/
│       ├── index.js     (weather fetch, timezone offset, Clay hookup)
│       └── config.js    (Clay settings definition)
```

### Watch side (C)
- Draw 7-segment digits: each digit = 7 segments (GPath polygons or filled rects). Lit segments use the digit color, unlit use the ghost color.
- Main time from `localtime`; honor 12h/24h; AM/PM marker.
- Second clock: take UTC time, add offset minutes received from phone, draw a smaller 7-segment clock plus city label.
- Steps: Health API (`health_service_sum_today(HealthMetricStepCount)`), with a health event handler to redraw.
- Battery: `battery_state_service`.
- Date and Day: from `strftime`.
- Temp: received from phone via AppMessage; persist last value with `persist_write_*` so it shows immediately on launch.
- Save settings with `persist_write_data`; load on startup.
- Use `tick_timer_service` at minute resolution.
- Handle unit/time-format changes without a reload.

### Phone side (PebbleKit JS)
- Clay config page (`config.js`) and handler wiring (`index.js`).
- Weather: use geolocation plus a no-API-key service (e.g. Open-Meteo), send temp in the chosen unit. Refresh every ~30 min and on watchface launch.
- Timezone offset: compute with `Intl.DateTimeFormat` using the chosen IANA zone (handles DST). Send offset in minutes. Recompute on each weather refresh and on launch, so DST changes are picked up.
- Send everything to the watch via AppMessage.

### package.json notes
- `"watchface": true`, `"targetPlatforms": ["emery"]`
- `"capabilities": ["configurable", "location_services"]` (check exact names against the current SDK docs; health access may need no capability on emery but verify)
- Define `messageKeys` for: TEMP, UNIT, THEME, SLOT_A to SLOT_D, TZ_OFFSET, TZ_LABEL, CLOCK_24H
- Dependency: `pebble-clay`

## Build order (suggested)
1. Scaffold the project; blank watchface builds and runs in the emery emulator.
2. 7-segment digit drawing; main time with ghost segments, themes.
3. Layout: frames, panels, slots with placeholder text.
4. Date, Day, Battery, Steps slots.
5. Second clock with a hard-coded offset.
6. Clay settings page, persist settings, apply theme/slots.
7. Phone-side weather and temp slot.
8. Phone-side timezone offset and city list.
9. Polish: AM/PM marker, ghost-dot dithering, edge cases (no GPS, no weather yet shows "--").

## Testing
- Emulator: `pebble install --emulator emery`
- Real watch: enable Developer Connection in the Pebble phone app, then `pebble install --phone <phone-ip>`
- Check: 12h/24h, DST zones, midnight rollover on the second clock (date mismatch is fine, time only), steps at 0, temp before first fetch.

## Open questions for the builder
- Confirm current SDK install method and exact `package.json` capability names against the latest Pebble developer docs.
- Decide ghost segment rendering: solid dim color vs dithered dots.
