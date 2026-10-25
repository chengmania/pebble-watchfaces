#!/usr/bin/env python3
"""Render InkSeg across its settings on an emulator and build a contact sheet.

Both clipping bugs found during the diorite port -- "OCTOBE..." and the
truncated sun times -- were only visible in a screenshot, so this makes that
sweep one command instead of a dozen.

Selects are sent as STRINGS on purpose: that is what Clay actually sends, and
reading them as ints was a real bug once.

    python3 tools/shots.py emery
    python3 tools/shots.py diorite --out /tmp/sheet.png
"""
import argparse
import json
import pathlib
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent
KEYS = json.loads((ROOT / "build/js/message_keys.json").read_text())

WEATHER = {"TEMP": 58, "WX_ICON": 1, "SUNRISE": 418, "SUNSET": 1092}

# name -> (int payload, string payload). Strings mirror Clay's <select> values.
SCENES = [
    # Settings persist on the emulator across runs, so the first scene asserts
    # a complete baseline rather than inheriting whatever the last run left.
    ("default",    {"SHOW_STEPS": 1, "SHOW_BATTERY": 1, "SHOW_HR": 1,
                    "SHOW_WX": 1, "SHOW_SUN": 1, "SHOW_WORLD": 1,
                    "LEADING_ZERO": 1},
                   {"INVERT": "0", "CLOCK_24H": "0", "UNIT": "1", "DATE_FMT": "0",
                    "WX_REFRESH": "30", "WORLD_OFFSET": "540", "WORLD_LABEL": "TOK"}),
    ("24h",        {}, {"CLOCK_24H": "1", "LEADING_ZERO": "1"}),
    ("inverted",   {}, {"INVERT": "1"}),
    ("short-date", {}, {"INVERT": "0", "DATE_FMT": "1"}),
    ("celsius",    {}, {"DATE_FMT": "0", "UNIT": "0"}),
    ("hr-off",     {"SHOW_HR": 0}, {"UNIT": "1"}),
    ("minimal",    {"SHOW_HR": 0, "SHOW_WORLD": 0, "SHOW_SUN": 0}, {}),
    # Leave the watch on the baseline, not on the last scene's overrides.
    ("_reset",     {"SHOW_HR": 1, "SHOW_WORLD": 1, "SHOW_SUN": 1},
                   {"INVERT": "0", "CLOCK_24H": "0", "DATE_FMT": "0", "UNIT": "1"}),
]


def run(*args, check=True):
    return subprocess.run(args, cwd=ROOT, check=check,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def send(platform, ints, strings):
    cmd = ["pebble", "send-app-message", "--emulator", platform]
    if ints:
        cmd.append("--int")
        cmd += [f"{KEYS[k]}={v}" for k, v in ints.items()]
    if strings:
        cmd.append("--string")
        cmd += [f"{KEYS[k]}={v}" for k, v in strings.items()]
    run(*cmd, check=False)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("platform", choices=["emery", "diorite"])
    ap.add_argument("--out", default=None)
    args = ap.parse_args()
    out = pathlib.Path(args.out) if args.out else ROOT / f"shots-{args.platform}.png"

    try:
        from PIL import Image, ImageDraw
    except ImportError:
        sys.exit("needs Pillow: pip install pillow")

    print("building...")
    run("pebble", "build")
    print(f"installing on {args.platform}...")
    run("pebble", "install", "--emulator", args.platform)
    time.sleep(4)
    send(args.platform, WEATHER, {})
    time.sleep(1)

    shots = []
    for name, ints, strings in SCENES:
        send(args.platform, ints, strings)
        if name.startswith("_"):
            continue  # state-only, not a panel on the sheet
        time.sleep(2)
        path = ROOT / f".shot-{name}.png"
        # The screenshot rides the emulated Bluetooth link and occasionally
        # times out, especially just after an install.
        for attempt in range(3):
            run("pebble", "screenshot", "--emulator", args.platform,
                "--no-open", str(path), check=False)
            if path.exists():
                break
            time.sleep(3)
        if path.exists():
            shots.append((name, Image.open(path).convert("RGB")))
            path.unlink()
            print(f"  {name}")
        else:
            print(f"  {name}: screenshot failed")

    if not shots:
        sys.exit("no screenshots captured")

    w, h = shots[0][1].size
    pad, label = 8, 14
    sheet = Image.new("RGB", (len(shots) * (w + pad) + pad, h + label + 2 * pad), "white")
    draw = ImageDraw.Draw(sheet)
    for i, (name, im) in enumerate(shots):
        x = pad + i * (w + pad)
        sheet.paste(im, (x, pad + label))
        draw.text((x, pad), name, fill="black")
    sheet.save(out)
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
