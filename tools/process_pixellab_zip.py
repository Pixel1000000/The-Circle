"""Turn a PixelLab character download zip into a DB32 64x64 atlas.

usage: process_pixellab_zip.py <zip> <out_char_dir> <tail_id> <char_id> <name> <group_id> <prompt> [preview.png]
"""
import io
import os
import sys
import zipfile
from PIL import Image

sys.path.insert(0, os.path.dirname(__file__))
from local_db32_quantize import load_palette, quantize_image  # noqa: E402
from build_enemy_atlas import build  # noqa: E402

DIRS = ["south", "south-east", "east", "north-east", "north", "north-west", "west", "south-west"]
CELL = 64


def opaque(im):
    return sum(1 for v in im.getchannel("A").getdata() if v > 0)


def fit_direction(frames):
    """One crop offset for all frames of a direction (no jitter), minimising clipped pixels."""
    w, h = frames[0].size
    if (w, h) == (CELL, CELL):
        return frames, 0
    cx, cy = (w - CELL) // 2, (h - CELL) // 2
    totals = [opaque(f) for f in frames]
    best = None
    for dy in range(-(cy), h - CELL - cy + 1):
        for dx in range(-(cx), w - CELL - cx + 1):
            x, y = cx + dx, cy + dy
            lost = sum(t - opaque(f.crop((x, y, x + CELL, y + CELL))) for f, t in zip(frames, totals))
            key = (lost, abs(dx) + abs(dy))
            if best is None or key < best[0]:
                best = (key, x, y)
    (lost, _), x, y = best
    return [f.crop((x, y, x + CELL, y + CELL)) for f in frames], lost


def main():
    zpath, out_dir, tail, cid, name, gid, prompt = sys.argv[1:8]
    preview = sys.argv[8] if len(sys.argv) > 8 else None
    z = zipfile.ZipFile(zpath)
    names = z.namelist()
    anim_root = next(n.split("/animations/")[0] + "/animations/" for n in names if "/animations/" in n)
    anim_name = sorted({n[len(anim_root):].split("/")[0] for n in names if n.startswith(anim_root)})
    assert len(anim_name) == 1, f"expected one animation, got {anim_name}"
    anim_root += anim_name[0] + "/"
    rot_root = anim_root.split("/animations/")[0] + "/rotations/"

    load = lambda p: Image.open(io.BytesIO(z.read(p))).convert("RGBA")
    palette, cache = load_palette(), {}
    os.makedirs(os.path.join(out_dir, "idle_db32"), exist_ok=True)
    os.makedirs(os.path.join(out_dir, "walk_db32"), exist_ok=True)

    idle_area, walk_area, total_lost = 0, 0, 0
    for d in DIRS:
        idle = load(f"{rot_root}{d}.png")
        assert idle.size == (CELL, CELL), f"idle {d} is {idle.size}"
        b = idle.getbbox()
        idle_area += (b[2] - b[0]) * (b[3] - b[1])
        quantize_image(idle, palette, cache).save(os.path.join(out_dir, "idle_db32", f"{d}.png"))

        frames = [load(f"{anim_root}{d}/frame_{i:03d}.png") for i in range(6)]
        frames, lost = fit_direction(frames)
        total_lost += lost
        for i, f in enumerate(frames):
            b = f.getbbox()
            walk_area += (b[2] - b[0]) * (b[3] - b[1]) / 6
            quantize_image(f, palette, cache).save(os.path.join(out_dir, "walk_db32", f"{d}_{i}.png"))

    print(f"clipped opaque px: {total_lost} | walk/idle bbox-area ratio: {walk_area / idle_area:.2f}")
    build(out_dir, cid, name, prompt, "low top-down", gid, tail)

    if preview:
        a = Image.open(os.path.join(out_dir, f"{tail}_Idle.png"))
        bg = Image.new("RGBA", a.size, (255, 255, 255, 255))
        bg.alpha_composite(a)
        bg.resize((a.width * 2, a.height * 2), Image.NEAREST).save(preview)

    import shutil
    shutil.rmtree(os.path.join(out_dir, "idle_db32"))
    shutil.rmtree(os.path.join(out_dir, "walk_db32"))


if __name__ == "__main__":
    main()
