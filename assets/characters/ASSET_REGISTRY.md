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
  (the real PixelLab export, replacing the earlier hand-generated placeholder)

## Enemies (in progress — see `enemies.json` for gameplay ids)

### forest_goblin_archer — GDD "Гоблин-лучник" (Биом 1 — Лес)
- Attempt 1: `character_id` `0c04b266-c271-4d8a-92d4-bb0d9df84361` — **REJECTED and deleted**
  at idle-rotation QA (didn't read as a goblin at all; see prior git history for the
  prompt and notes).
- Attempt 2: `character_id` `5ed0c9d2-bf5e-45c1-b959-6c6306339612` — idle rotations +
  full 8-dir walk generated, DB32-quantized (0 off-palette pixels across all 56
  frames), loader-verified, and saved to disk. **REJECTED by the user** after seeing
  the actual files: reads as a human in a green hood/cap, not a goblin creature at
  all — no goblin facial/body anatomy came through despite the green skin. Character
  deleted from PixelLab, files removed from `assets/characters/forest_goblin_archer/`.
- Attempt 3 (current): `character_id` `119548ec-a641-44fb-9cfd-6f31af188ebc`. User
  supplied a reference image of a classic fantasy goblin (bald, huge pointed ears,
  hooked nose, hunched, wrinkled green skin) and asked for that creature dressed as
  an archer. Prompt: "лысый зеленокожий гоблин-монстр, не человек, нечеловеческое
  существо: сутулая спина, тощее жилистое тело, огромная лысая голова, гигантские
  остроконечные уши торчком, длинный крючковатый нос, маленькие острые клыки,
  морщинистая бугристая ярко-зелёная кожа, глубоко посаженные жёлтые глаза; одет как
  лесной лучник — рваная кожаная безрукавка, натягивает короткий составной лук,
  колчан со стрелами за спиной" (text_guidance_scale=16, custom proportions:
  head_size 1.3, arms_length 1.1, legs_length 0.85, shoulder_width 0.7, hip_width
  0.75, to push a hunched/goblin-like silhouette away from the default human build).
  QA pending.

### winter_ice_goblin — GDD "Ледяной гоблин" (Биом 3 — Зима)
- `character_id`: not started
- Idle rotations: not started
- Walk `animation_group_id`: not started
