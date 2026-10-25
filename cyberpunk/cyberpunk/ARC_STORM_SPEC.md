# Arc Storm: Cyberpunk Watchface Spec for Claude Code

An electric-blue cyberpunk Pebble watchface with lightning effects, glitch digits, and arc charge rings around the time. Flashy on demand, static and readable the rest of the time.

Working directory: `/home/chengmania/Nextcloud/projects/pebble/cyberpunk`

## Target
- Platform: **emery** (Pebble Time 2), 200x228, color (64-color palette), heart-rate sensor
- Project type: watchface (`"watchface": true`)
- Scaffold (if not already done): `pebble new-project --c --javascript --ai cyberpunk`

## Look

```
┌──────────────────────┐
│ ⚡72°  ☀6:58 ☾18:12   │  top row: weather, sunrise, sunset
│  ╭──────────────╮    │
│ ╱   12 : 47      ╲   │  big angular digits, cyan with dark-blue shadow offset
│ │   SUN OCT 25    │  │  date under the time
│ ╲                ╱   │
│  ╰──────────────╯    │  arc rings around the time block
│ ♥ 72   👣 8,431  ▮▮▮▯ │  bottom row: heart rate, steps, battery
└──────────────────────┘
```

### Palette (Pebble GColor names)
- Background: `GColorBlack`
- Primary glow: `GColorCyan`, `GColorElectricBlue`
- Secondary: `GColorVividCerulean`, `GColorPictonBlue`
- Shadow / dim: `GColorOxfordBlue`, `GColorDukeBlue`
- Highlights / lightning core: `GColorWhite`
- Warning (low battery, high HR): `GColorMagenta` or `GColorShockingPink` as a small accent

Define colors in a theme struct so alternate themes (e.g. "Toxic Green", "Synth Pink") can be added later.

### Digits
- Custom angular "cyber" digits drawn in code (GPath polygons or segment-style with beveled ends), not a stock font.
- Glow effect: draw a dark-blue copy offset by 2 px (shadow), then the cyan digit on top, optionally a 1 px white inner highlight.
- Glitch state: during animation, randomly offset horizontal slices of the digits by a few pixels and swap colors briefly.

### Arc rings
Two concentric arcs framing the time (use `graphics_fill_radial` / `graphics_draw_arc`):
- **Outer ring: steps progress** toward a daily goal (goal configurable, default 10,000).
- **Inner ring: watch battery** percentage.
- Unfilled portion drawn in dim blue; filled portion in cyan/electric blue; turns magenta when battery < 20%.
- Small tick marks along the rings for a HUD feel.

## Data (all shown)
| Data | Source |
|---|---|
| Time | `localtime`, 12h/24h setting |
| Date | `strftime` (e.g. SUN OCT 25) |
| Steps | `health_service_sum_today(HealthMetricStepCount)` |
| Heart rate | `health_service_peek_current_value(HealthMetricHeartRateBPM)`; show `--` if unavailable |
| Battery | `battery_state_service`; show charging bolt when charging |
| Weather (temp + condition icon) | Phone JS, Open-Meteo (no API key), geolocation |
| Sunrise / sunset | Phone JS, Open-Meteo daily `sunrise`/`sunset`, sent as local HH:MM or minutes-since-midnight |

## Animations ("the flashy part")
Keep animations short and event-driven to protect battery. Use `AppTimer` frame loops (~30 fps max) or the Pebble `Animation` API, and stop timers when done.

1. **Wrist flick (accel tap service): Lightning strike, ~1 s**
   - A jagged lightning bolt (randomly generated polyline from top to bottom, with a branch or two) flashes white/cyan across the screen.
   - Brief full-screen flash (one or two frames of inverted/bright background).
   - Digits glitch (slice offsets, color swaps) for a few frames, then settle back to normal.
   - Bolt shape randomized each time.
2. **Each minute change: Spark run, ~0.5 s**
   - A bright spark dot travels along the outer arc ring from 0 to the current step progress, leaving a fading trail.
3. **Heart pulse**
   - The ♥ icon scales/brightens in rhythm with the current BPM.
   - Default: pulses only for ~10 s after a wrist flick, then goes static (battery). Setting option: "Always" or "On flick".
4. **Charging (optional polish)**
   - While charging, small electric crackle along the battery ring every few seconds.

Provide a setting to disable animations entirely ("Calm mode").

## Settings page (Clay)
- **Theme:** Arc Storm (blue) (others can be added later)
- **Slots:** top row (3 slots) and bottom row (3 slots), each selectable from: Weather, Sunrise, Sunset, Sunrise+Sunset, Heart rate, Steps, Battery, Date, Off
  - Defaults: top = Weather, Sunrise, Sunset; bottom = Heart rate, Steps, Battery
- **Rings:** outer ring = Steps / Battery / Off; inner ring = Steps / Battery / Off
- **Step goal:** number, default 10000
- **Units:** °F / °C
- **Time format:** 12h / 24h
- **Animations:** Full / Calm (no animations)
- **Heart pulse:** On flick / Always
- **Weather refresh:** 15 / 30 / 60 min (default 30)

Persist all settings on the watch with `persist_write_data` and apply without reload.

## Architecture

```
cyberpunk/
├── package.json         (watchface, emery, messageKeys, capabilities, pebble-clay)
├── src/
│   ├── c/
│   │   ├── main.c       (window, services, settings, message handling)
│   │   ├── digits.c/.h  (cyber digit drawing + glitch rendering)
│   │   ├── rings.c/.h   (arc rings, ticks, spark)
│   │   ├── fx.c/.h      (lightning bolt generator, flash, animation timers)
│   │   └── slots.c/.h   (slot rendering, icons)
│   └── pkjs/
│       ├── index.js     (weather, sunrise/sunset, Clay hookup, AppMessage)
│       └── config.js    (Clay settings definition)
```

### Watch side notes
- Minute-resolution `tick_timer_service`; redraw static layer each minute.
- Use separate layers: static layer (frame, rings, slots), digits layer, FX overlay layer (only marked dirty during animations).
- `accel_tap_service_subscribe` for the flick trigger; debounce so repeated taps don't stack animations.
- Health events (`health_service_events_subscribe`) to refresh steps/HR.
- Cache last weather and sun times in persistent storage so they show instantly on launch.
- Draw small pixel icons in code (bolt, sun, moon, heart, footprints, battery) to avoid image resources, or use small PNG resources if cleaner.

### Phone side notes
- Open-Meteo request: `current=temperature_2m,weather_code` and `daily=sunrise,sunset`, `timezone=auto`, `forecast_days=1`, temperature unit per setting.
- Map `weather_code` to a small set of icon IDs (clear, cloudy, rain, snow, storm, fog).
- Send temp, icon ID, sunrise and sunset (minutes since midnight) via AppMessage.
- Refresh on launch, on settings change, and on the configured interval.

### package.json notes
- `"watchface": true`, `"targetPlatforms": ["emery"]`
- `"capabilities": ["configurable", "location", "health"]` (verify exact names against the current SDK; heart rate needs health permission)
- `messageKeys`: TEMP, WX_ICON, SUNRISE, SUNSET, UNIT, CLOCK_24H, THEME, TOP_1..TOP_3, BOT_1..BOT_3, RING_OUTER, RING_INNER, STEP_GOAL, ANIM_MODE, HR_PULSE, WX_REFRESH
- Dependency: `pebble-clay`

## Build order (suggested)
1. Scaffold, convert to watchface, blank black screen runs on emery emulator.
2. Cyber digits with shadow glow; time + date centered.
3. Arc rings with tick marks driven by steps and battery.
4. Top and bottom slots with icons; health + battery data.
5. Clay settings page, persistence, slot/ring/units/format options.
6. Phone-side weather and sunrise/sunset.
7. Lightning strike animation on wrist flick (bolt generator, flash, digit glitch).
8. Minute spark run; heart pulse; charging crackle.
9. Calm mode, edge cases (no HR, no GPS, no weather yet → `--`), battery check of animation timers.

## Testing
- Emulator: `pebble build && pebble install --emulator emery`
- Simulate flick: `pebble emu-tap`; battery: `pebble emu-battery --percent 15`; health data may be limited in the emulator, so test HR/steps on the real watch.
- Real watch: Developer Connection in the Pebble phone app, then `pebble install --phone <phone-ip>`
- Take emulator screenshots (`pebble screenshot`) to review visuals.

## Open questions for the builder
- Verify capability names and heart-rate API availability on the active SDK (4.33.1).
- Confirm `pebble emu-*` command names with `pebble --help`.
- Tune animation frame rate vs battery on the real watch.
