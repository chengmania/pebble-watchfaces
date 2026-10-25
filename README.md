# Pebble Watchfaces

A collection of Pebble watchfaces, each a self-contained Pebble SDK 3 project.

| Watchface | Directory | Platforms |
| --- | --- | --- |
| **InkSeg** | [`epaper/`](epaper) | emery, diorite |
| **Arc Storm** | [`cyberpunk/cyberpunk/`](cyberpunk/cyberpunk) | emery |
| **LCD Watch** | [`myfirstwatch/lcd-watch/`](myfirstwatch/lcd-watch) | emery |

## Building

Each project builds independently with the Pebble SDK:

```sh
cd epaper        # or cyberpunk/cyberpunk, myfirstwatch/lcd-watch
npm install
pebble build
pebble install --emulator emery
```

Build output (`build/`, `*.pbw`) and `node_modules/` are not tracked.

## Project layout

Each watchface follows the standard Pebble layout:

- `src/c/` — C source for the watchface itself
- `src/pkjs/` — PebbleKit JS (configuration, weather, phone-side logic)
- `resources/` — fonts and images
- `package.json` — Pebble app metadata (UUID, target platforms, resources)
- `*_SPEC.md` — design spec for the face
