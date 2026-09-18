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
- Attempt 2 (current): `character_id` `5ed0c9d2-bf5e-45c1-b959-6c6306339612`. Prompt:
  "зелёный лесной гоблин-дозорный с острыми клыками и остроконечными ушами,
  ярко-зелёная шершавая кожа, натягивает короткий составной лук, колчан со стрелами
  за спиной, рваная тёмно-коричневая кожаная безрукавка" (text_guidance_scale=12).
  Canvas 92x92 (standard-mode auto-expansion, same as attempt 1).
  - Idle rotations (8 dir): generated, visually inspected via inline previews for all
    8 directions — consistent green skin, hooded/capped green outfit, dark boots,
    single-color black outline, no visible seams or off-model direction. **ACCEPTED
    by my own visual QA.**
  - `reduce_colors` against `assets/palettes/db32.png`: job `de16f191-eed6-4346-893e-d3720081b242`,
    ran on all 8 rotation frames together (shared palette). Inspected all 8 resulting
    frames inline — colors consolidated to a small consistent set (dark green skin/cap,
    brown boots, black outline), no stray off-palette speckling visible at this
    resolution. **ACCEPTED by my own visual QA**, pending the user's confirmation
    per the ТЗ's requirement not to self-approve on this iteration.
  - Walk `animation_group_id`: not started — waiting on go-ahead before spending more
    generations on animation.
  - **Files: NOT saved to `assets/characters/forest_goblin_archer/` yet.** This
    session's outbound network is restricted to an allowlist that does not include
    PixelLab's asset hosts (`backblaze.pixellab.ai`, `api.pixellab.ai`) - direct
    `curl` downloads were rejected by the org's egress proxy with 403 (policy
    denial, not a transient error - see `/root/.ccr/README.md`). The MCP server's
    own image previews rendered fine (that's how the QA above was done), but there
    is currently no tool available in this session that returns raw file bytes I
    can write to disk, or a way to push binary content through the GitHub file-write
    tool (it only accepts UTF-8 text). **Needs a session with PixelLab network
    access, or the user downloading the PNGs from the URLs below and committing
    them manually.**
  - Rotation URLs (post-`reduce_colors`, via `get_image` download links, index 0-7 =
    south, east, north, west, south-east, north-east, north-west, south-west):
    `https://api.pixellab.ai/mcp/images/de16f191-eed6-4346-893e-d3720081b242/download?index=<0-7>`
  - Pre-`reduce_colors` rotation URLs (raw generation output) are on
    `get_character(character_id="5ed0c9d2-bf5e-45c1-b959-6c6306339612")`.

### winter_ice_goblin — GDD "Ледяной гоблин" (Биом 3 — Зима)
- `character_id`: not started
- Idle rotations: not started
- Walk `animation_group_id`: not started
