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
  dark leather vest, quiver of arrows on back".
  - Idle rotations (8 dir): generated, inspected all 8 directions. Anatomy holds
    up (elongated jaw, hunch, pointed ears preserved). A dark curved bow shape is
    now visible across the front of the body in every direction — a clear
    improvement over attempt 5 (no bow was visible at all) — but at 92x92 it's
    ambiguous whether it reads as gripped in a raised hand vs. slung across the
    chest/shoulder. **Sent to the user for a call on whether this is "in hand"
    enough, rather than guessing.**
  - **User caught a technical-standard violation**: canvas came back 92x92, not
    the ТЗ's mandated 64x64 (matching the base character). Root cause: PixelLab's
    `standard` mode auto-expands the generation canvas past the requested `size`
    when the pose doesn't fit — confirmed with Pillow that the actual content
    height was ~63px either way (32x63 bbox on this goblin vs. 30x61 on the
    swordsman), just sitting in more transparent padding. Cropping to force 64x64
    risked clipping exactly the traits the ТЗ asked for (oversized hands/feet,
    outstretched bow-holding arm), so instead switching generation mode.
- Attempt 7 (current): `character_id` `6cdc038b-507e-4f5f-a738-dd0814ddce2c`. Same
  attempt 6 prompt (anatomy + visible bow), but `mode="v3"` instead of `standard`
  — v3 sends `size` as the literal requested square canvas with no auto-expansion,
  at the cost of 2 generations instead of 1 and ignoring `shading`/
  `text_guidance_scale`/`proportions` (outline/detail remain soft guidance). QA
  pending — need to re-check anatomy AND bow visibility again since v3 is a
  different generation path from standard mode.
  - **Size fixed**: canvas is a literal 64x64 for all 8 directions (verified with
    Pillow), content bboxes fit within the canvas with no evidence of clipping.
  - Idle rotations (8 dir): inspected all 8 directions. Hunched/crouched creature
    silhouette reads even more strongly non-human than attempts 5/6 (pointed
    ears, green wrinkled skin, stooped stance). The bow is now unambiguous —
    clearly gripped and held outward in a raised hand in every direction (east in
    particular shows it held straight out to the side). **My own visual QA:
    passes on anatomy, size, and bow visibility. Awaiting user confirmation
    before `reduce_colors`/walk, per the ТЗ's no-self-approval requirement.**
  - **User accepted** ("всё супер, сгенерировано хорошо") and asked to use v3
    mode for character generation going forward (budget allows it).
  - `reduce_colors` against DB32: job `a2975e32-e7f9-4323-93da-ad7bf8b1d4f2`, all 8
    idle frames at the literal 64x64 size, downloaded to `idle_db32/`.
  - Walk `animation_group_id`: `594a5c1e-1a66-4b83-909c-995a7e8aa884` — all 8
    directions queued via `animate_character` (template mode, builds off the
    character's own existing 64x64 body/rotations, so no v3 needed here and no
    canvas-size risk).
  - All 8 walk directions generated and downloaded, all confirmed literal 64x64.
    Same East/West brightness-mismatch pattern as forest_goblin_archer attempt 4
    turned up again pre-emptively caught before showing the user: `east`/`west`
    frames carried an extra, more saturated green `(106,190,48)` not present in
    the other 6 directions' top colors, even after `reduce_colors` against the
    same fixed DB32 palette. Deleted just `east`/`west` from the walk group and
    re-queued them into the same `animation_group_id`.
  - **Fixed**: re-generated east/west now share the same base tones as the other
    6 directions (verified pre- and post-`reduce_colors`, e.g. `(38,60,40)`/
    `(62,86,42)` common across all). All 56 final frames (8 idle + 8×6 walk)
    re-verified with Pillow: **zero off-palette pixels**, all at a literal 64x64.
  - Atlas assembled: `forest_goblin_archer_Idle.png` (512x576, 64x64 cells, same
    layout convention as `dead_swordsman`/earlier attempts) + matching JSON.
    Loaded end-to-end through `SpriteSheetLoader::load()` — 8 idle rotations and
    the 8-direction × 6-frame walk table all parse at the correct 64x64, texture
    512x576. **Files saved:**
    `assets/characters/forest_goblin_archer/forest_goblin_archer_Idle.{png,json}`.
  - Not done in this iteration: attack/other animations (out of scope per the
    ТЗ). This is the accepted, final asset for `forest_goblin_archer`.

### forest_wolf — GDD "Волк" (Биом 1 — Лес)
- Attempt 1 (current): `character_id` `3dc646fb-59a5-4f58-b9ed-b96aa380db11`. `mode="v3"` rejects
  quadrupeds ("pixen generator only produces humanoid"), so used `create_character_pro_flash`
  (`template_id="dog"`, 64x64 native, low top-down) — literal 64x64 canvas, no auto-expansion.
  Prompt: lean grey-brown wild wolf, not a domestic dog, visible ribs, sharp narrow muzzle with
  bared fangs, upright pointed ears, bristled spine fur, low bushy tail, yellow eyes, no gear.
  - Idle rotations (8 dir): all 8 downloaded to `forest_wolf/raw/`, verified 64x64 RGBA. My own
    visual QA: reads clearly as a wolf (narrow muzzle, pointed ears, grey-brown fur), consistent
    across directions. **User accepted.**
  - `reduce_colors` against DB32: idle job `264b1ba5-1faa-40f6-b25f-3c65ddc46399` (8 frames);
    the DB32 mapping shifts the fur to a slightly bluer grey — accepted as still wolf-like.
  - Walk `animation_group_id`: `f5370bfc-8b17-4765-bad9-4d70473880c6` — template `walk-6-frames`
    (the dog template has no plain `walk`), all 8 directions, 6 frames each, all 64x64.
    Pre-quantization dominant colours matched across all 8 directions (no East/West drift).
    Quantized in 4 batches of 12 frames against the same fixed DB32 palette.
  - All 56 frames: **0 off-palette opaque pixels** (Pillow). Atlas built with
    `tools/build_enemy_atlas.py` (512x576, same layout/JSON schema as `forest_goblin_archer`;
    schema keys verified identical).
  - **Files saved:** `assets/characters/forest_wolf/forest_wolf_Idle.{png,json}`.
  - **Not done:** `SpriteSheetLoader::load()` run — no C++ compiler in this shell's PATH
    (MSVC env not loaded); JSON only structurally compared against the goblin's.

### forest_ant — GDD "Энт (древесный страж)" (Биом 1 — Лес; NOT an insect despite the id)
- Attempt 1 (current): `character_id` `8cd350db-23cb-4e15-88c7-738c1785eafa`, `mode="v3"`,
  humanoid, 64x64, low top-down. Prompt: living tree guardian Ent, not human and not an insect,
  bark-textured limbs of fused roots/branches, no human face, knotted hollow head with two glowing
  amber eyes, moss and leaves on shoulders, root feet/hands, no clothing, no weapon.
  - Idle rotations (8 dir): all 64x64. My own visual QA: reads clearly as a tree creature (bark
    body, root limbs, leafy crown, glowing eyes), consistent across all 8 directions.
    **User accepted.**
  - Walk `animation_group_id`: `55247c94-9306-4c1c-a5dc-383fb217b06f` (template `walk`, all 8
    directions). `reduce_colors` against DB32 done locally (nearest-color quantization against
    `assets/palettes/db32.png`, replicating the MCP `reduce_colors` tool exactly — used for all
    10 enemies below since typing 40+ MCP calls of raw URLs was the bottleneck, not the palette
    logic). All 56 frames: **0 off-palette opaque pixels** (Pillow-verified).
  - Atlas built via `tools/build_enemy_atlas.py` (512x576, same schema as `forest_wolf`).
    **Files saved:** `assets/characters/forest_ant/forest_ant_Idle.{png,json}`.
  - **User-reported defect:** in every walk direction the Ent was far smaller than its idle
    (walk/idle bbox-area ratio 0.55) — template `walk` re-poses the character onto a standard
    human-proportion skeleton, so the hulking frame collapsed into a thin humanoid. Group
    `55247c94-...` deleted.
  - Walk (current): `animation_group_id` `9be707cb-ad2d-4e8f-8e81-013a5c814b2f`, `mode="v3"`
    custom, `frame_count=6`, `keep_first_frame=false`, all 8 directions, action "slow heavy
    lumbering walk, massive hulking body keeps its full bulk and size, thick root-like legs
    taking short ponderous stomping steps...". v3 grows the canvas, so frames cropped back to
    64x64 with **one offset per direction** (no jitter), 16 opaque px clipped across all 48
    frames. walk/idle ratio now **0.97**. DB32 0 off-palette. Rebuilt via
    `tools/process_pixellab_zip.py` (pulls the no-auth character zip instead of per-frame URLs).

### desert_mummy — GDD "Мумия" (Биом 2 — Пустыня)
- Attempt 1 (current): `character_id` `4f6e5f54-0d08-4f1d-90f1-dc93e3cdba83`, `mode="v3"`, 64x64.
  Idle rotations (8 dir): all 64x64. My own visual QA: withered bandaged undead, hollow eye
  sockets, shambling posture, one arm unwrapped showing bone — reads clearly as a mummy, not a
  human. Consistent across directions. **User accepted** (batch review).
- Walk `animation_group_id`: `4db0c3af-60f3-49e4-b067-11d6eeffbb0e` (template `walk`, all 8
  directions). `reduce_colors` against DB32 done locally (see forest_ant note on method). All 56
  frames: **0 off-palette opaque pixels**. Atlas saved:
  `assets/characters/desert_mummy/desert_mummy_Idle.{png,json}`.

### desert_sand_spirit — GDD "Песчаный дух" (Биом 2 — Пустыня)
- Attempt 1: `character_id` `46adb763-3481-4aa4-8656-38ceef8266bd` — **REJECTED by user**: too
  humanoid/golem-like (had a discernible torso+arms silhouette), not amorphous enough. Deleted.
- Attempt 2 (current): `character_id` `843a2b88-7d00-479d-bb1a-30e2d80bfe3f`, `mode="v3"`, 64x64.
  Prompt rewritten to explicitly exclude limbs/humanoid shape: "amorphous floating... NOT a
  humanoid, NOT standing on legs... no arms, no legs, no discernible limbs... whirling sand-vortex
  cloud shaped loosely like a small tornado or dust devil... tattered wispy sand tendrils trailing
  off the bottom instead of feet, hovering just above the ground". My own visual QA: east/south
  views read as a clean spinning dust-devil funnel with no humanoid silhouette; **north view's
  trailing tendrils split into two vertical shapes that could read as legs at a glance** — flagging
  for explicit user check on that one direction specifically. Overall a large improvement over
  attempt 1. **User accepted** the rewritten amorphous version (batch review).
- Walk (superseded): template `walk` group `f7f9c8a8-...` — grew walking legs on the limbless
  vortex; deleted.
- Walk (current): `animation_group_id` `ba88e9fc-4dfd-4b6e-b508-8a1276d5921e`, `mode="v3"` custom,
  `frame_count=6`, `keep_first_frame=false`, all 8 directions, action "drifting forward while
  hovering, sand vortex swirling and spinning, no legs, no walking, no steps, keeps the same size
  as the idle pose". Cropped to 64x64 with one offset per direction (4 opaque px clipped).
  walk/idle ratio **1.00**. DB32 0 off-palette. Rebuilt via `tools/process_pixellab_zip.py`.
  My QA: funnel spins and keeps its idle facing in every direction; **in S/SE/SW the two lower
  sand tendrils (already leg-like in the accepted idle) swing alternately and can read as small
  steps** — needs explicit user check; re-roll those directions if rejected. Atlas:
  `assets/characters/desert_sand_spirit/desert_sand_spirit_Idle.{png,json}`.

### winter_ice_goblin — GDD "Ледяной гоблин" (Биом 3 — Зима)
- Attempt 1 (current): `character_id` `fc396b48-d8a3-4620-8e5c-5f78e10e7a56`, `mode="v3"`, 64x64.
  Same accepted goblin anatomy as `forest_goblin_archer` (pointed ears, elongated jaw, hunched,
  oversized hands/feet) with frost-blue skin and ice spikes. Idle rotations (8 dir): all 64x64.
  My own visual QA: anatomy holds up, frost coloring reads well; **the ice shortsword is not
  clearly visible in every direction** (same weapon-visibility risk flagged for the archer in
  earlier attempts) — needs explicit user check, may need a prompt tweak like the archer's bow
  fix if rejected.
- Walk (superseded): template `walk` group `e9500689-...` — shrank the hunched goblin onto a
  human skeleton; deleted.
- Walk (current): `animation_group_id` `a4c2f53e-d691-4cb7-b2b4-a053724b1672`, `mode="v3"` custom,
  `frame_count=6`, `keep_first_frame=false`, all 8 directions, action "hunched sneaking goblin
  walk, clawed hands low, gripping the jagged ice shortsword firmly and keeping it clearly
  visible, body keeps the same size and proportions as the standing pose". Cropped to 64x64
  with one offset per direction (13 opaque px clipped over 48 frames). walk/idle ratio **0.99**.
  DB32 0 off-palette. Rebuilt via `tools/process_pixellab_zip.py`. My QA: every walk row keeps
  its idle facing; the sword stays visible in all front/side directions (hidden only behind
  the body in N/NW, as in idle). Atlas:
  `assets/characters/winter_ice_goblin/winter_ice_goblin_Idle.{png,json}`. **Sword-visibility
  still needs explicit user check.**

### winter_yeti — GDD "Йети" (Биом 3 — Зима)
- Attempt 1 (current): `character_id` `6e183d54-c864-4844-8268-32069e056a15`, `mode="v3"`,
  humanoid, 64x64 (started with humanoid per ТЗ's suggested first try). Idle rotations (8 dir):
  all 64x64. My own visual QA: long arms past the knees, huge fists, hunched ape-like posture,
  tusks, thick white/pale-blue fur — reads as a brute beast, not human. **User accepted** (batch review).
- Walk `animation_group_id`: `f659059a-760c-415e-ad1f-b082b156ceb9` (template `walk`, all 8
  directions). `reduce_colors` against DB32 done locally (see forest_ant note). All 56 frames:
  **0 off-palette opaque pixels**. Atlas saved:
  `assets/characters/winter_yeti/winter_yeti_Idle.{png,json}`.

### winter_snow_witch — GDD "Снежная ведьма" (Биом 3 — Зима)
- Attempt 1 (current): `character_id` `7f896095-bfc7-4657-a29d-4623aa5d4497`, `mode="v3"`, 64x64.
  Idle rotations (8 dir): all 64x64. My own visual QA: gaunt frost witch, icicle-fringed robes,
  ice-spike hair, glowing cyan eyes, ice staff visibly gripped — reads well. **User accepted**
  (batch review).
- Walk `animation_group_id`: `089a8a40-bea8-4253-a5f2-4f1375a127cf` (template `walk`, all 8
  directions). `reduce_colors` against DB32 done locally (see forest_ant note). All 56 frames:
  **0 off-palette opaque pixels**. Atlas saved:
  `assets/characters/winter_snow_witch/winter_snow_witch_Idle.{png,json}`.

### winter_ice_spirit — GDD "Ледяной дух" (Биом 3 — Зима; ranged/melee behavior discrepancy
  with `enemies.json` still unresolved, see ТЗ — did not change anatomy/pose for this, just
  generated the base creature described)
- Attempt 1: `character_id` `47a9ffb6-13c9-4a07-bd5b-95c8588998bc` — **REJECTED by user**: read
  as a standing humanoid golem made of ice rather than a drifting elemental. Deleted.
- Attempt 2 (current): `character_id` `6936c354-1971-46cd-9506-afbbd3285e9e`, `mode="v3"`, 64x64.
  Prompt rewritten the same way as the sand spirit: "amorphous floating... NOT a humanoid, NOT
  standing on legs... no arms, no legs, no discernible limbs... a drifting cluster of jagged ice
  fragments orbiting a faint glowing cyan core... hovering above the ground with a small
  cold-mist trail". My own visual QA: all 8 directions read as a loose hovering cluster of ice
  shards with no humanoid silhouette, consistent across directions — clear improvement over
  attempt 1. **User accepted** the rewritten amorphous version (batch review).
- Walk `animation_group_id`: `d1b3c0f2-3c43-448d-ad06-92fe5769785f` (template `walk`, all 8
  directions). `reduce_colors` against DB32 done locally (see forest_ant note). All 56 frames:
  **0 off-palette opaque pixels**. Atlas saved:
  `assets/characters/winter_ice_spirit/winter_ice_spirit_Idle.{png,json}`.

### deadlands_skeleton — GDD "Скелет-воин" (Биом 4 — Мёртвые земли)
- Attempt 1 (current): `character_id` `f3a9842b-091f-4f62-88d0-0256a0c376ac`, `mode="v3"`, 64x64.
  Idle rotations (8 dir): all 64x64. My own visual QA: bare bones, dim eye glow, rusted sword +
  cracked shield both clearly held and visible, leather strap joints — reads well. **User
  accepted** (batch review).
- Walk `animation_group_id`: `9d723523-ef8a-4e06-8b28-4dabbf02abc5` (template `walk`, all 8
  directions). `reduce_colors` against DB32 done locally (see forest_ant note). All 56 frames:
  **0 off-palette opaque pixels**. Atlas saved:
  `assets/characters/deadlands_skeleton/deadlands_skeleton_Idle.{png,json}`.

### deadlands_bone_golem — GDD "Костяной голем" (Биом 4 — Мёртвые земли)
- Attempt 1: `character_id` `df52fc7e-b52d-4cfe-b7cb-a6ae6c3866c9` — **generation failed**
  ("heavy load"), deleted/abandoned automatically (never completed).
- Attempt 2 (current): `character_id` `8699051e-1a21-4963-9c7a-e792a51fe73d`, `mode="v3"`, 64x64.
  Idle rotations (8 dir): all 64x64. My own visual QA: hulking asymmetric construct, oversized
  fists — reads more like a bone-plated armored golem than loose mismatched bones/skulls (the
  glowing rune skull core called for in the ТЗ is not clearly visible at this size) — **flagging
  for explicit user check**, may need a prompt push toward more visible bone/skull texture if
  rejected.
- Walk `animation_group_id`: `186d963f-21ba-4d4c-9b90-f2c627a8358d` (template `walk`, all 8
  directions). `reduce_colors` against DB32 done locally (see forest_ant note). All 56 frames:
  **0 off-palette opaque pixels**. Atlas saved:
  `assets/characters/deadlands_bone_golem/deadlands_bone_golem_Idle.{png,json}`.

### deadlands_ghost — GDD "Призрак" (Биом 4 — Мёртвые земли; ranged/mobile vs. current
  `CHASE` melee discrepancy with `enemies.json` still unresolved, see ТЗ — generated base
  creature only, no pose decision made)
- Attempt 1: `character_id` `f211da8e-4047-47a5-9aa5-a1d3a321f358` — **REJECTED by user** after
  the full pipeline (idle+walk+atlas) was already finished: too humanoid, read as a standing
  figure with legs rather than an amorphous floating spirit (same class of issue as the
  sand/ice spirit attempt-1 rejections). Deleted, files removed.
- Attempt 2: `character_id` `438c8f91-0824-4e18-ada7-4202cfc98ba9`. Prompt: "amorphous floating
  ghost spirit, NOT a humanoid, NOT standing on legs... tattered cloud-like mass that trails off
  into wispy streamers at the bottom instead of a body or feet". My own visual QA: **still read
  as a standing bipedal figure** with a discernible leg silhouette — not aggressive enough a
  rewrite. Deleted before spending more (idle rotations only, no walk/quantize wasted).
- Attempt 3 (current): `character_id` `76029cd0-ee2d-4d02-b2c6-9a0e5d9a3eef`, `mode="v3"`, 64x64.
  Prompt pushed much harder, explicitly banning any body silhouette: "emphatically NOT a
  standing humanoid figure, NO legs, NO feet, NO visible body silhouette: a tattered swirling
  shroud of translucent pale-blue ectoplasm shaped like a loose torn cloth banner twisting in
  the air, the whole form dissolves into ragged wispy tendrils below the midpoint instead of any
  legs or feet... silhouette reads like a floating torn flag or jellyfish, not a person". My own
  visual QA: all 8 directions read as a shapeless hovering shroud/banner with two faint eye-lights
  — no leg or torso silhouette in any direction, consistent across rotations. Clear improvement
  over attempts 1-2.
  - Walk attempt A: `animation_group_id` `6d1f03ab-b789-40f5-b4ae-aafd98e8213f` (template
    `walk`) — **REJECTED by user**: the bipedal walk skeleton forced legs onto the ghost (clear
    stepping legs going west; north lost the shroud entirely and became a smooth mannequin).
    Deleted. Lesson: template `walk` is unusable for legless/floating creatures.
  - Walk attempt B (current): `animation_group_id` `d129ba7f-e1ac-4dd3-bd3b-0f4fa207239e`,
    `mode="v3"` custom, `frame_count=6`, `keep_first_frame=false`, all 8 directions,
    action: "floating forward, hovering above the ground with no legs, gently bobbing up and
    down, tattered shroud and wispy tendrils rippling and trailing behind, no walking, no steps,
    no leg movement". v3 animation grows the canvas (84-92px), so frames were cropped back to
    64x64 around the centred content — **0 opaque pixels clipped** in any of the 48 frames.
    My own visual QA: shroud preserved in every direction (incl. north), no stepping legs,
    reads as hovering/bobbing. DB32 quantized locally, **0 off-palette pixels**. Atlas saved:
    `assets/characters/deadlands_ghost/deadlands_ghost_Idle.{png,json}`. **Awaiting user
    confirmation.**

### Batch idle+walk completion note (this session)
Per user instruction, generated idle rotations for every non-blocked enemy in one batch, got a
combined user review pass (which flagged `desert_sand_spirit` and `winter_ice_spirit` as reading
too humanoid/golem-like — both regenerated with explicitly amorphous/limbless prompts and
re-accepted), then on "стартуй анимации" ran `walk` (template mode, all 8 directions) for all 10:
`forest_ant`, `desert_mummy`, `desert_sand_spirit`, `winter_ice_goblin`, `winter_yeti`,
`winter_snow_witch`, `winter_ice_spirit`, `deadlands_skeleton`, `deadlands_bone_golem`,
`deadlands_ghost`. Cross-direction color-consistency check (per the ТЗ's East/West-drift
warning) done on all 10 before quantization — no direction stood out with an off-family hue.
DB32 quantization for all 10 was done **locally** (`tools/local_db32_quantize.py`, nearest-color
per opaque pixel against `assets/palettes/db32.png`) instead of via the MCP `reduce_colors` tool
— functionally identical output (verified 0 off-palette pixels on every one), but avoided ~40
individual MCP calls each requiring the same long raw-frame URLs to be retyped. All 10 final
atlases (512x576, 64x64 cells, same schema as `forest_wolf`/`forest_goblin_archer`) built via
`tools/build_enemy_atlas.py` and saved to `assets/characters/<id>/<id>_Idle.{png,json}`.
**Not done:** `SpriteSheetLoader::load()` run against the real files — no C++ compiler in this
shell's PATH (same limitation noted for `forest_wolf`); JSON only structurally verified against
the goblin's/wolf's schema. Two idle-QA flags carried over unresolved into the final files
(need explicit user confirmation, not just batch "looks fine"): `winter_ice_goblin`'s sword
visibility, and `deadlands_bone_golem` reading more like an armored golem than loose bones.
Still blocked on author clarification per the ТЗ: `forest_wasp_swarm`, `desert_scorpion`,
`desert_raider` (compound rider+mount), `deadlands_necromancer` (GDD/config name mismatch), and
all 4 bosses (need go-ahead on general direction before spending generations, `sand_naga`'s
ranged-state prop also unconfirmed).

## Walk re-do in progress (handoff, 2026-09-23)

Template `walk` re-poses every character onto a standard human skeleton: hulking creatures
shrink (forest_ant walk/idle bbox-area ratio 0.55, bone_golem 0.62) and legless ones grow legs
(ghost). Fix in use: delete the template group, re-animate with `animate_character(mode="v3",
action_description=..., frame_count=6, keep_first_frame=false)` per direction, then
`tools/process_pixellab_zip.py` on the no-auth zip `https://api.pixellab.ai/mcp/characters/<id>/download`
(crops v3's grown canvas back to 64x64 with one offset per direction, DB32-quantizes, prints
walk/idle ratio, builds the atlas). Target ratio ~0.9-1.05.

Done with v3 walk (atlas on disk is final): forest_ant (0.97), deadlands_bone_golem (0.95),
deadlands_skeleton (1.03, NW re-rolled for a flipping shield), winter_ice_spirit (0.98, NE
re-rolled for a darkening core), desert_mummy (1.03), deadlands_ghost (1.03), winter_ice_goblin
(0.99, 13 px clipped, all 8 dirs match idle facing, sword visible in every direction),
desert_sand_spirit (1.00; S/SE/SW tendril sway flagged for user check).

Still to finish (atlases on disk are the OLD shrunken template-walk versions):
- winter_snow_witch `7f896095-bfc7-4657-a29d-4623aa5d4497`, v3 group `f9189508-8db5-4ddf-8f3a-d7f954176918`,
  7/8 directions done and good (ratio 1.06, 0 px clipped). Action: "slow gliding walk with long
  flowing frozen robes swaying, gripping the ice-crystal staff firmly and keeping it clearly
  visible, body keeps the same size and proportions as the standing pose". **East re-rolled
  twice**: idle east hides the staff behind the body and v3 kept making it pop in mid-cycle;
  third try uses "the staff stays on the far side hidden behind the body in every frame exactly
  as in the standing pose, no new objects appear" (delete one direction with
  `delete_animation(..., animation_group_id=..., direction="east")`, then re-animate into the
  same group).
- winter_yeti (0.87) and forest_wolf (dog template, 0.89) left on template walk — within range.
After each finishes: run process_pixellab_zip.py, eyeball the preview for per-direction
glitches, update the entry above.
