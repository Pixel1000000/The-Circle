import json, sys, os
from PIL import Image

DIRS_IDLE = ["south","south-east","east","north-east","north","north-west","west","south-west"]

def build(char_dir, char_id, name, prompt, view, anim_group_id, tail_id):
    pal = Image.open(os.path.join(os.path.dirname(__file__), "..", "assets", "palettes", "db32.png")).convert("RGBA")
    palette = {p[:3] for p in pal.getdata() if p[3] > 0}
    sheet = Image.new("RGBA", (512, 576), (0, 0, 0, 0))
    off = 0
    for i, d in enumerate(DIRS_IDLE):
        im = Image.open(f"{char_dir}/idle_db32/{d}.png").convert("RGBA")
        sheet.paste(im, (i * 64, 0))
        off += sum(1 for p in im.getdata() if p[3] > 0 and p[:3] not in palette)
        for f in range(6):
            im = Image.open(f"{char_dir}/walk_db32/{d}_{f}.png").convert("RGBA")
            sheet.paste(im, (f * 64, (i + 1) * 64))
            off += sum(1 for p in im.getdata() if p[3] > 0 and p[:3] not in palette)
    out_png = f"{char_dir}/{tail_id}_Idle.png"
    sheet.save(out_png)
    rows = [{"row": 0, "type": "rotations", "frame_count": 8, "directions": DIRS_IDLE}]
    for i, d in enumerate(DIRS_IDLE):
        rows.append({"row": i + 1, "type": "animation", "frame_count": 6, "animation": "walk",
                     "animation_group_id": anim_group_id, "direction": d})
    js = {"character": {"id": char_id, "name": name, "prompt": prompt, "size": {"width": 64, "height": 64},
                        "directions": 8, "view": view},
          "spritesheet": {"path": f"{tail_id}_Idle.png", "cell_size": {"width": 64, "height": 64},
                          "sheet_size": {"width": 512, "height": 576}, "columns": 8, "pivot": "cell-center", "rows": rows},
          "export_version": "1.0"}
    json.dump(js, open(f"{char_dir}/{tail_id}_Idle.json", "w"), indent=2)
    print("off-palette opaque pixels:", off, "| saved", out_png)
