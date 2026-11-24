# GeoWorks 1.x Test icon: monitor with test-pattern colour bars.
# c48x30 is the user's own GTS1ICON.PCX (light magenta = transparent);
# the other five sizes are derived from it.
import os
from PIL import Image
T = -1
HERE = os.path.dirname(os.path.abspath(__file__))
src = Image.open(os.path.join(HERE, 'GTS1ICON.PCX'))
W0, H0 = src.size
base = [[(T if src.getpixel((x, y)) == 13 else src.getpixel((x, y))) for x in range(W0)] for y in range(H0)]

def remap(img, cols, rows):
    return [[img[r][c] if (r is not None and c is not None) else T for c in cols] for r in rows]

# 64x40: widen the six colour bars (5 -> 7 px), heighten the screen
cols = [None, None] + list(range(0, 8))
for b in range(6):
    s = 8 + 5 * b
    cols += [s, s, s + 1, s + 2, s + 3, s + 4, s + 4]
cols += list(range(38, 48)) + [None, None]
rows = [0, 0] + list(range(1, 6))
rows += [6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15, 15]   # 10 -> 13
rows += [16, 16, 17, 18, 18]                                # 3 -> 5
rows += [19, 20, 21, 21, 22, 22, 23, 24, 25, 25, 26, 27, 28, 29, 29]
assert len(cols) == 64 and len(rows) == 40, (len(cols), len(rows))
c64 = remap(base, cols, rows)

# monochrome: 1 = black. Bars become different patterns.
PAT = {15: lambda x, y: 0, 14: lambda x, y: (x % 4 == 0 and y % 2 == 0),
       12: lambda x, y: (x + y) % 2 == 0, 10: lambda x, y: x % 2 == 0,
       11: lambda x, y: y % 2 == 0, 9: lambda x, y: (x + y) % 4 != 0,
       4: lambda x, y: 1, 1: lambda x, y: (x + y) % 2 == 0, 2: lambda x, y: 1,
       5: lambda x, y: x % 2 == 0, 6: lambda x, y: 1, 3: lambda x, y: (x + y) % 2 == 0,
       7: lambda x, y: 0, 8: lambda x, y: 1, 0: lambda x, y: 1}
def mono(img):
    return [[T if v == T else (0 if PAT[v](x, y) else 15) for x, v in enumerate(row)] for y, row in enumerate(img)]

def small(w, h):
    # tiny monitor: outline, screen with vertical stripes, stand
    img = [[T] * w for _ in range(h)]
    mw = int(h * 1.5); x0 = (w - mw) // 2; x1 = x0 + mw - 1
    y1 = h - 4
    for y in range(0, y1 + 1):
        for x in range(x0, x1 + 1):
            edge = y in (0, y1) or x in (x0, x1)
            inner = 2 <= y <= y1 - 2 and x0 + 2 <= x <= x1 - 2
            img[y][x] = 0 if edge else (0 if inner and ((x - x0) // 2) % 2 else 15)
    cx = (x0 + x1) // 2
    for x in range(cx - 2, cx + 3): img[h - 3][x] = 0
    for x in range(cx - 1, cx + 2): img[h - 2][x] = 0
    for x in range(cx - 5, cx + 6): img[h - 1][x] = 0
    return img

out = {
    'c64x40': (64, 40, True, c64),
    'm64x40': (64, 40, False, mono(c64)),
    'c48x30': (48, 30, True, base),
    'm48x30': (48, 30, False, mono(base)),
    'm64x18': (64, 18, False, small(64, 18)),
    'm48x14': (48, 14, False, small(48, 14)),
}

if __name__ == '__main__':
    PAL = [(0,0,0),(0,0,170),(0,170,0),(0,170,170),(170,0,0),(170,0,170),(170,85,0),(170,170,170),
           (85,85,85),(85,85,255),(85,255,85),(85,255,255),(255,85,85),(255,85,255),(255,255,85),(255,255,255)]
    tiles = []
    for name, (w, h, color, img) in out.items():
        im = Image.new('RGB', (w, h), (200, 200, 200))
        for y in range(h):
            for x in range(w):
                if img[y][x] != T:
                    im.putpixel((x, y), PAL[img[y][x]])
        tiles.append(im.resize((w * 5, h * 5), Image.NEAREST))
    sheet = Image.new('RGB', (sum(t.width for t in tiles) + 16 * len(tiles), max(t.height for t in tiles)), (255, 255, 255))
    x = 0
    for t in tiles:
        sheet.paste(t, (x, 0)); x += t.width + 16
    sheet.save(os.path.join(HERE, 'preview.png'))
