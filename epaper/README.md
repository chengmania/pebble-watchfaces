# InkSeg

A stark, pure black-and-white Pebble watchface for **emery** (Pebble Time 2,
200x228) and **diorite** (Pebble 2, 144x168), inspired by the Watchy
7-segment e-paper face. Only `GColorBlack` and `GColorWhite`, no animation,
no dithering.

```
 AM
 5:29          huge drawn 7-segment time, lit segments only
 SUNDAY        day, bold
 OCTOBER 25    month + date
 ===========   3px rule
 steps 7,300            [battery]
 heart --               TOK 6:29P
 wx 58°F        ^6:58  v6:12
```

## Building & running

```sh
npm install                      # pebble-clay
pebble build
pebble install --emulator emery     # or --emulator diorite
pebble screenshot --emulator emery --no-open shot.png
pebble install --phone <phone-ip>   # real watch, Developer Connection on
```

Add `--vnc` to every emulator command in a headless environment.

## Project layout

```
src/c/main.c          window, services, settings, AppMessage, layout
src/c/seg.c/.h        7-segment digit drawing (beveled, no ghost segments)
src/c/icons.c/.h      1-bit icon drawing
src/c/icons_data.inc  generated icon bitmaps
tools/genicons.py     icon bitmap generator
tools/shots.py        settings-sweep contact sheet
src/pkjs/index.js     Open-Meteo weather + sunrise/sunset, Clay hookup
src/pkjs/config.js    Clay settings page
resources/fonts/      IBM Plex Sans Condensed SemiBold (OFL)
```

Icon bitmaps are generated. Edit the ASCII art in `tools/genicons.py` and run
it from the project root to rewrite `src/c/icons_data.inc`:

```sh
python3 tools/genicons.py
```

## Two screens

Digit geometry lives in `src/c/seg.h` and the block is centred from
`SEG_BLOCK_W`, so the time scales rather than being repositioned by hand:

| | emery | diorite |
|---|---|---|
| Screen | 200x228 | 144x168 |
| Digit | 38x76, 10px segments | 27x50, 7px segments |
| Date / row / small type | Plex 28 / 20 / 16 | Plex 20 / 16 / 14 |
| Row pitch | 20px | 19px |

Diorite's right-hand column is too narrow to hold both a temperature and two
sun times on one line, so the sun times ride with the heart rate there and the
world time sits beside the weather. Everything still fits in three rows;
`SUN_ON_HR_ROW` selects the arrangement. The icons are shared at 1x on both.

## Settings (Clay)

| Setting | Options |
|---|---|
| Invert | Black on white (default) / White on black |
| Date format | `SUNDAY / OCTOBER 25` or `SUN 25 OCT` |
| Time format | 12h / 24h |
| Leading zero (24h) | On / Off |
| Data rows | Steps, Battery, Heart rate monitor, Weather, Sunrise/Sunset |
| World time | On/off, 5-character label, UTC offset |
| Units | °F / °C |
| Weather refresh | 15 / 30 / 60 minutes |

Rows reflow upward to close gaps when items are switched off. Settings and the
last weather reading are kept in persistent storage, so the face is correct the
moment it loads.

The **heart rate monitor** toggle both hides the row and cancels this app's
heart-rate sample-period request. An app cannot power the sensor down
system-wide; with the toggle off the watch simply returns to its own
battery-saving sampling schedule.

## Freshness and connectivity

A wrong number is worse than no number, so values expire:

- **Temperature** falls back to `--` once the reading is more than 6 hours old
  (`WX_STALE_SECS`), and the condition icon disappears with it.
- **Sunrise / sunset** show `--` unless they belong to today's date.
- **Heart rate** shows `--` until the sensor returns a reading above zero.
- A crossed-out Bluetooth mark appears beside the battery while the phone is
  unreachable.

The **watch** decides when weather is due, rather than a timer in PebbleKit JS
that would not survive a Bluetooth drop or a JS restart. On each minute tick it
compares `fetched_at` against the refresh interval and sends `FETCH`; the phone
answers, retrying a failed request at 20s, 60s and 180s before waiting for the
next ask. Reconnecting triggers an immediate refresh.

Temperature conversion is a **display step**, not a stored mutation: the watch
records which unit the phone fetched in (`temp_is_f`) and converts only when
rendering. Converting in place meant a repeated settings message could compound
58°F into 136°F.

A colour change dirties the **root** layer as well as the canvas. Marking only
the canvas left the previous paper colour on screen, so Invert appeared not to
take until the app was reinstalled.

The settings struct carries a `version` byte, checked on load, so an
unrecognised layout starts clean instead of reading garbage out of flash.

Day and month names come from tables in `main.c`, not `strftime`: the bundled
font is a trimmed subset, and a localised name would render as holes. The face
is English-only by design. The world-time label is filtered to the same subset.

### Heart rate shows `--`

Heart rate needs a system permission that is separate from step tracking: steps
can work perfectly while heart rate is denied. Turn on heart rate monitoring in
the watch's **Settings -> Health**.

A watchface can only *request* a sampling period; it cannot power the sensor.
When that request is refused the face logs the reason once, so `pebble logs`
distinguishes the three cases that otherwise all render as `--`:

```
heart rate unavailable (mask=2): heart rate monitoring is off in the watch's Health settings
```

`mask=2` is `NoPermission`, `4` is `NotSupported` (no sensor), and a reading
that simply has not arrived yet reports neither.

## Testing

`tools/shots.py` renders the face across its settings on an emulator and writes
a contact sheet, which is how the clipped `OCTOBE...`, the truncated sun times
and a clipped `AM` were all caught:

```sh
python3 tools/shots.py emery
python3 tools/shots.py diorite --out /tmp/sheet.png
```

The first scene asserts a complete baseline, because emulator settings persist
between runs. Selects are sent as **strings**, which is what Clay really sends.

## Data sources

Time and date from `localtime`/`strftime`; steps and heart rate from the health
service; battery from `battery_state_service`; temperature, condition and
sunrise/sunset from Open-Meteo via PebbleKit JS and phone geolocation. Anything
not yet available renders as `--`.
