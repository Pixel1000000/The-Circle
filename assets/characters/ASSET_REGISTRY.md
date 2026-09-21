# PixelLab asset registry

Tracks every character generated through the PixelLab MCP pipeline, so
`character_id`/`animation_group_id` values are recoverable without digging
through chat history. Update this file whenever a new character/state/
animation is generated or re-generated.

Visual standard for every entry below: 64px cell, low top-down view, single
color black outline, basic shading, medium detail, DB32 palette (enforced
via `reduce_colors` against `assets/palettes/db32.png`).

## Playable characters

### dead_swordsman
- `character_id`: `d2139b93-2f81-48af-893d-32d23dd575f5`
- State: `Idle` (8-direction idle rotations)
- Walk animation `animation_group_id`: `5c45dd2e-949d-4abd-9083-36835d61c410` (6 frames × 8 directions)
- Files: `assets/characters/dead_swordsman/dead_swordsman_Idle.{png,json}`
  (PNG is currently a hand-generated placeholder matching the real JSON
  layout — swap in the real PixelLab export when available)

## Enemies (in progress — see `enemies.json` for gameplay ids)

### forest_goblin_archer — GDD "Гоблин-лучник" (Биом 1 — Лес)
- Attempt 1: `character_id` `0c04b266-c271-4d8a-92d4-bb0d9df84361` — **REJECTED and deleted**
  at idle-rotation QA (didn't read as a goblin at all; see prior git history for the
  prompt and notes).
- Attempt 2 (ACCEPTED): `character_id` `5ed0c9d2-bf5e-45c1-b959-6c6306339612`. Prompt:
  "зелёный лесной гоблин-дозорный с острыми клыками и остроконечными ушами,
  ярко-зелёная шершавая кожа, натягивает короткий составной лук, колчан со стрелами
  за спиной, рваная тёмно-коричневая кожаная безрукавка" (text_guidance_scale=12).
  Canvas 92x92 (standard-mode auto-expansion from the requested 64px).
  - Idle rotations (8 dir): generated and visually inspected — consistent green skin,
    hooded/capped green outfit, dark boots, single-color black outline, no seams or
    off-model direction. **ACCEPTED.**
  - Walk `animation_group_id`: `8c942d58-adea-4aff-beed-58d13a1e5b46` — full 8/8
    directions generated via `animate_character` (template `walk`, 6 frames/direction):
    5 directions (south, south-east, east, north-east, north) generated first, then
    the remaining 3 (west, north-west, south-west) appended to the same group.
    Visually inspected sample frames per direction — consistent with idle art and
    with each other. **ACCEPTED.**
  - `reduce_colors` against `assets/palettes/db32.png`: run in 7 batches (1 for the
    8 idle frames, 6 for the 8×6 walk frames — batched to stay under reduce_colors'
    per-call pixel budget), all against the same fixed DB32 palette image so every
    batch quantizes onto identical colors. Verified programmatically with Pillow: all
    56 final frames (8 idle + 8×6 walk) have **zero** off-palette opaque pixels.
  - Loader check: `SpriteSheetLoader::load()` against the real files parses correctly
    — 8 idle rotations at 92x92, walk table with all 8 directions × 6 frames each.
  - **Files saved:**
    `assets/characters/forest_goblin_archer/forest_goblin_archer_Idle.{png,json}`
    (736x828 atlas: row 0 = 8 idle rotations, rows 1-8 = walk per direction, 92x92
    cells, built from the actual PixelLab-generated + DB32-quantized frames — not a
    placeholder).
  - Not done in this iteration: attack/other animations (out of scope per the ТЗ).

### winter_ice_goblin — GDD "Ледяной гоблин" (Биом 3 — Зима)
- `character_id`: not started
- Idle rotations: not started
- Walk `animation_group_id`: not started
