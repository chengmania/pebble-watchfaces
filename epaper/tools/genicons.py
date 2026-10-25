ICONS = {}

# ----------------------------------------------------------------- helpers

def blank(w, h):
    return [["."] * w for _ in range(h)]

def stamp(grid, pat, col, row):
    for dy, line in enumerate(pat):
        for dx, c in enumerate(line):
            if c == "#":
                grid[row + dy][col + dx] = "#"

def px(grid, col, row):
    grid[row][col] = "#"

def rowrun(grid, row, c0, c1):
    for c in range(c0, c1 + 1):
        grid[row][c] = "#"

def finish(name, grid):
    ICONS[name] = ["".join(r) for r in grid]

# ----------------------------------------------------------------- steps

FOOT = [
 ".#####.",
 "#######",
 "#######",
 "#######",
 ".#####.",
 "..###..",
 "..###..",
 ".#####.",
 ".#####.",
 "..###..",
]
g = blank(16, 16)
stamp(g, FOOT, 0, 0)
stamp(g, FOOT, 9, 6)
finish("steps", g)

# ----------------------------------------------------------------- heart

ICONS["heart"] = [
 "................",
 "...####..####...",
 ".######..######.",
 ".##############.",
 ".##############.",
 ".##############.",
 "..############..",
 "...##########...",
 "....########....",
 ".....######.....",
 "......####......",
 ".......##.......",
 "................",
 "................",
 "................",
 "................",
]

# ----------------------------------------------------------------- bolt

ICONS["bolt"] = [
 "...##",
 "..##.",
 ".##..",
 "#####",
 "..##.",
 ".##..",
 "##...",
]

# ----------------------------------------------------------------- weather

DISC = [(4, 6, 8), (5, 5, 9), (6, 4, 10), (7, 4, 10), (8, 4, 10), (9, 5, 9), (10, 6, 8)]

def sun(g):
    for r, c0, c1 in DISC:
        rowrun(g, r, c0, c1)
    for r in (0, 1):
        px(g, 7, r)
    for r in (13, 14):
        px(g, 7, r)
    for c in (0, 1):
        px(g, c, 7)
    for c in (13, 14):
        px(g, c, 7)
    for (r, c) in [(2, 2), (3, 3), (2, 12), (3, 11), (12, 2), (11, 3), (12, 12), (11, 11)]:
        px(g, c, r)

g = blank(16, 15)
sun(g)
finish("wx_clear", g)

CLOUD = [
 "...#####...",
 "..#######..",
 ".#########.",
 "###########",
 "###########",
 ".#########.",
]

SMALLSUN = [
 "..#..",
 "#####",
 "..#..",
]

g = blank(16, 15)
stamp(g, CLOUD, 2, 4)
finish("wx_cloudy", g)

# Partly cloudy: a sun disc with a cloud drifting across its lower left.
g = blank(16, 15)
for r, c0, c1 in [(1, 10, 12), (2, 9, 13), (3, 8, 14), (4, 8, 14), (5, 9, 13), (6, 10, 12)]:
    rowrun(g, r, c0, c1)
stamp(g, CLOUD, 0, 7)
finish("wx_partly", g)

g = blank(16, 15)
stamp(g, CLOUD, 2, 2)
for c in (4, 8, 12):
    for r in (9, 10, 11):
        px(g, c, r)
    px(g, c - 1, 12)
finish("wx_rain", g)

FLAKE = [
 "#.#.#",
 ".###.",
 "#####",
 ".###.",
 "#.#.#",
]
g = blank(16, 15)
stamp(g, CLOUD, 2, 1)
stamp(g, FLAKE, 2, 9)
stamp(g, FLAKE, 9, 9)
finish("wx_snow", g)

g = blank(16, 15)
stamp(g, CLOUD, 2, 1)
stamp(g, ICONS["bolt"], 5, 8)
finish("wx_storm", g)

g = blank(16, 15)
for r, (c0, c1) in zip((3, 6, 9, 12), ((1, 13), (2, 14), (0, 12), (2, 13))):
    rowrun(g, r, c0, c1)
    rowrun(g, r + 1, c0, c1)
finish("wx_fog", g)

# ----------------------------------------------------------------- bluetooth

BT_RUNE = [
 "..#..",
 "..##.",
 "#.#.#",
 ".###.",
 "..#..",
 ".###.",
 "#.#.#",
 "..##.",
 "..#..",
]
g = blank(9, 9)
stamp(g, BT_RUNE, 2, 0)
for i in range(9):          # strike through: disconnected
    px(g, i, 8 - i)
finish("bt_off", g)

# ----------------------------------------------------------------- arrows

ICONS["arrow_up"] = [
 "...#...",
 "..###..",
 ".#####.",
 "#######",
 "...#...",
 "...#...",
 "...#...",
 "...#...",
 "...#...",
]
ICONS["arrow_down"] = list(reversed(ICONS["arrow_up"]))

# ----------------------------------------------------------------- emit

def emit(name, rows):
    h = len(rows)
    w = max(len(r) for r in rows)
    vals = []
    for r in rows:
        r = r.ljust(w, ".")
        v = 0
        for i, c in enumerate(r):
            if c == '#':
                v |= 1 << (15 - i)
        vals.append(v)
    body = ",\n  ".join("0x%04X" % v for v in vals)
    return ("static const uint16_t s_%s_rows[%d] = {\n  %s\n};\n"
            "const IconBitmap ICON_%s = { %d, %d, s_%s_rows };\n\n"
            % (name, h, body, name.upper(), w, h, name))

out = []
for n, r in ICONS.items():
    out.append(emit(n, r))
open("src/c/icons_data.inc", "w").write("".join(out))

for n in ("wx_clear", "wx_partly", "wx_cloudy", "wx_rain", "wx_snow", "wx_storm", "wx_fog"):
    print(n)
    print("\n".join(ICONS[n]))
    print()
