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
- `character_id`: `0c04b266-c271-4d8a-92d4-bb0d9df84361`
- Status: idle rotations generating — QA pending
- Idle rotations (8 dir): not yet accepted
- Walk `animation_group_id`: not started
- Files: not yet saved to `assets/characters/forest_goblin_archer/`

### winter_ice_goblin — GDD "Ледяной гоблин" (Биом 3 — Зима)
- `character_id`: not started
- Idle rotations: not started
- Walk `animation_group_id`: not started
