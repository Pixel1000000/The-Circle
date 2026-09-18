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
  at idle-rotation QA. Prompt: "поджарый лесной гоблин-дозорный с остроконечными ушами,
  кривым коротким луком и колчаном стрел за спиной, землисто-зелёная шершавая кожа,
  рваная кожаная безрукавка". Generated at 92x92 canvas (standard mode auto-expanded
  from the requested 64px) instead of the base character's flat 64x64. Visually reads
  as a plain pale-skinned human in a dark vest and blue jeans — no green goblin skin,
  no visible pointed ears, no bow/quiver came through at this size. Does not read as
  a goblin at all next to the base dead_swordsman.
- Status: awaiting go-ahead on a retry prompt before spending more generations
- Idle rotations (8 dir): not accepted
- Walk `animation_group_id`: not started
- Files: nothing saved to `assets/characters/forest_goblin_archer/` yet

### winter_ice_goblin — GDD "Ледяной гоблин" (Биом 3 — Зима)
- `character_id`: not started
- Idle rotations: not started
- Walk `animation_group_id`: not started
