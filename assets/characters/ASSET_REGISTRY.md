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
  **REJECTED**: lost the green skin entirely — came back as a pale/grey bald human
  in a t-shirt and shorts, no bow/quiver visible. Deleted.
- Attempt 4 (current): `character_id` `f6214c67-9111-4f79-9574-715fdfd97cee`. Switched
  to an English prompt (the underlying model likely has stronger "goblin" priors in
  English than a Russian description built from individual anatomical terms) and
  dropped the custom proportions (suspected of confusing attempt 3 rather than
  helping). Prompt: "green-skinned fantasy goblin monster, bald head, huge pointed
  bat-like ears, long hooked nose, small sharp fangs, hunched posture, skinny wiry
  body, warty wrinkled bright green skin, yellow eyes, dressed as a forest archer:
  tattered dark leather vest, drawing a short recurve bow, quiver of arrows on back"
  (text_guidance_scale=10, default proportions).
  - Idle rotations (8 dir): **ACCEPTED.** English "goblin" wording worked — bald
    head, pointed ears, green wrinkled skin, dark leather archer gear with visible
    quiver straps, consistent across all 8 directions and consistent with the
    reference image the user supplied.
  - `reduce_colors` against DB32: job `6a0471fa-6b3c-4984-b48d-58b21665af96`, all 8
    idle frames, downloaded to `idle_db32/`.
  - Walk `animation_group_id`: `c900ea1e-efe9-4d6a-ab6b-2b8714b481c3` — all 8
    directions generated in one call (template `walk`, 6 frames/direction).
    Visually inspected sample frames per direction — consistent goblin anatomy and
    archer gear, matches the idle art. **ACCEPTED.**
  - `reduce_colors` against DB32: 8 idle frames (job `6a0471fa-...`) + 8 walk
    directions × 6 frames each, one `reduce_colors` batch per direction (7 more
    jobs), all against the same fixed DB32 palette. Verified programmatically with
    Pillow: all 56 final frames (8 idle + 8×6 walk) have **zero** off-palette
    opaque pixels.
  - Loader check: `SpriteSheetLoader::load()` against the real files parses
    correctly — 8 idle rotations at 92x92, walk table with all 8 directions × 6
    frames each.
  - **Files saved:**
    `assets/characters/forest_goblin_archer/forest_goblin_archer_Idle.{png,json}`
    (736x828 atlas, same layout convention as dead_swordsman/attempt 2 — built from
    the actual PixelLab-generated + DB32-quantized frames, not a placeholder).
  - Not done in this iteration: attack/other animations (out of scope per the ТЗ).
  - **User-reported defect (post-merge):** walk east/south-east frames' base skin
    color was visibly brighter/more saturated than the other 6 directions even
    after DB32 quantization — confirmed by comparing pre-quantization pixel values:
    east/south-east's own `animate_character` jobs rendered a more saturated green
    (~(67,153,29)/(70,139,34)) than the other directions' (~(64,107,33)), so
    `reduce_colors` (correctly, given a fixed external palette) mapped them onto
    different DB32 slots than the rest — a genuine per-job generation variance, not
    a quantization bug. Fix: deleted just the `east`/`south-east` directions from
    the walk group (`delete_animation(..., direction=...)`) and re-queued them via
    `animate_character` into the same `animation_group_id`, hoping for a closer
    color match on retry.
    **Fixed**: the re-generated east/south-east now share the same base skin
    color `(64,107,33)` pre-quantization as every other direction (vs. the
    original `(67,153,29)`/`(70,139,34)`), and after `reduce_colors` all 8
    directions' frame-0 dominant color matches `(75,105,47)`. Re-verified all 56
    frames still have zero off-palette pixels, rebuilt the atlas with the fixed
    frames, and re-ran it through `SpriteSheetLoader` (still parses correctly).
- Attempt 4 **fully superseded** per an updated ТЗ: the new spec explicitly calls
  out that a goblin generated from a human-anatomy base reads as "a human painted
  green" and requires the classic-fantasy anatomical traits to be spelled out in
  the prompt, not just color/clothing — pointed ears, elongated jaw/fangs, hunched
  posture, and **disproportionately large hands/feet** (this last one was missing
  from attempt 4's prompt). Character `f6214c67-...` deleted, files removed;
  redoing from scratch as attempt 5. (GDD gives no anatomical detail for this
  enemy beyond the name "Гоблин-лучник" — per the ТЗ, that would normally mean
  stopping to ask, but the ТЗ text itself already spells out the required goblin
  anatomy, so no clarification was needed here.)
- Attempt 5 (current): `character_id` `20e07abe-4db8-4295-932c-dfc3ce09844b`.
  Prompt: "green-skinned fantasy goblin monster, emphatically not human, distinct
  inhuman creature anatomy: large pointed bat-like ears, elongated protruding
  lower jaw with jutting fangs, hunched stooped posture with a curved spine,
  disproportionately large gnarled hands with long clawed fingers, oversized bare
  feet, skinny wiry hunched body, warty wrinkled bright green skin, sunken yellow
  eyes, bald knobby head; dressed as a forest archer: tattered dark leather vest,
  drawing a short recurve bow, quiver of arrows on back" (text_guidance_scale=10,
  default proportions, standard mode — no reference_image/style_character_id to
  the human base character).
  - Idle rotations (8 dir): generated, inspected all 8 directions. Silhouette now
    reads as a distinct creature — elongated/pointed skull-and-jaw shape unlike a
    human head, visible hunch in the side profiles (east/north-east/south-west),
    dark leather archer gear with quiver on back. My own visual read: passes the
    "not a human painted green" bar.
  - **User feedback**: anatomy accepted ("принимаю частично"), but the bow needs
    to be visibly held in hand — the "drawing a short recurve bow" wording didn't
    render a clearly visible bow in any direction. Character deleted, redone as
    attempt 6.
- Attempt 6 (current): `character_id` `e137f92a-784b-4bc7-bd49-e62f51a82a50`. Same
  accepted anatomy description, equipment clause changed to "gripping a wooden
  longbow firmly in one clawed hand, bow held out and clearly visible, tattered
  dark leather vest, quiver of arrows on back". QA pending.

### winter_ice_goblin — GDD "Ледяной гоблин" (Биом 3 — Зима)
- `character_id`: not started
- Idle rotations: not started
- Walk `animation_group_id`: not started
