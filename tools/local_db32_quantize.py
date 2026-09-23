"""Quantize raw PixelLab frames to the DB32 palette locally (no MCP calls),
replicating reduce_colors(palette_image=db32.png): nearest-color per opaque
pixel, alpha preserved as-is (0 stays fully transparent).
"""
import sys
import os
from PIL import Image

HERE = os.path.dirname(__file__)
PALETTE_PATH = os.path.join(HERE, "..", "assets", "palettes", "db32.png")


def load_palette():
    im = Image.open(PALETTE_PATH).convert("RGBA")
    colors = []
    seen = set()
    for p in im.getdata():
        if p[3] > 0 and p[:3] not in seen:
            seen.add(p[:3])
            colors.append(p[:3])
    return colors


def nearest(rgb, palette, cache):
    if rgb in cache:
        return cache[rgb]
    best = min(palette, key=lambda c: (c[0]-rgb[0])**2 + (c[1]-rgb[1])**2 + (c[2]-rgb[2])**2)
    cache[rgb] = best
    return best


def quantize_image(im, palette, cache):
    im = im.convert("RGBA")
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            nr, ng, nb = nearest((r, g, b), palette, cache)
            px[x, y] = (nr, ng, nb, 255 if a > 0 else 0)
    return im


def quantize_dir(src_dir, dst_dir, palette, cache):
    os.makedirs(dst_dir, exist_ok=True)
    n = 0
    for fname in os.listdir(src_dir):
        if not fname.endswith(".png"):
            continue
        im = Image.open(os.path.join(src_dir, fname))
        out = quantize_image(im, palette, cache)
        out.save(os.path.join(dst_dir, fname))
        n += 1
    return n


def verify_off_palette(dst_dir, palette):
    pal_set = set(palette)
    off = 0
    for fname in os.listdir(dst_dir):
        if not fname.endswith(".png"):
            continue
        im = Image.open(os.path.join(dst_dir, fname)).convert("RGBA")
        for p in im.getdata():
            if p[3] > 0 and p[:3] not in pal_set:
                off += 1
    return off


if __name__ == "__main__":
    char_dir = sys.argv[1]
    palette = load_palette()
    cache = {}
    n_idle = quantize_dir(os.path.join(char_dir, "idle_raw"), os.path.join(char_dir, "idle_db32"), palette, cache)
    n_walk = quantize_dir(os.path.join(char_dir, "walk_raw"), os.path.join(char_dir, "walk_db32"), palette, cache)
    off1 = verify_off_palette(os.path.join(char_dir, "idle_db32"), palette)
    off2 = verify_off_palette(os.path.join(char_dir, "walk_db32"), palette)
    print(f"{char_dir}: idle {n_idle} frames (off-palette {off1}), walk {n_walk} frames (off-palette {off2})")
