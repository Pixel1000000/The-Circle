# ТЗ: генерация спрайтов оставшихся врагов и боссов через PixelLab MCP

## Контекст

Продолжение пайплайна из «ТЗ: генерация спрайтов обычных врагов через PixelLab MCP»
(и его правки «Требование к образу — классический фэнтезийный стиль»). Первая
итерация (`forest_goblin_archer`) завершена и принята автором проекта после
7 попыток. Этот документ переносит все выводы той итерации в конкретное
задание на оставшихся **15 обычных врагов** и **4 боссов**.

Готовый референс: `assets/characters/forest_goblin_archer/` +
`assets/characters/ASSET_REGISTRY.md` (полная история попыток, включая
отклонённые — полезно как пример того, что именно не проходит QA).

## Технический стандарт (не отступать)

- Разрешение **буквально 64×64px**, низкий top-down ракурс, единая чёрная
  обводка, базовый шейдинг, DB32-палитра — идентично базовому персонажу
  (`d2139b93-...`) и `forest_goblin_archer`.
- **Использовать `mode="v3"` в `create_character`**, а не `standard`. В
  `standard`-режиме PixelLab сам расширяет холст сверх запрошенного `size`,
  если поза не помещается (проверено: гоблин с сутулостью и вытянутой рукой
  ушёл на холст 92×92 вместо 64×64). `v3` передаёт `size` буквально, без
  авто-расширения. Стоимость — 2 генерации вместо 1 (бюджет позволяет).
  `v3` игнорирует `shading`/`text_guidance_scale`/`proportions` — это
  некритично, `outline`/`detail` остаются soft guidance.
- У врагов нет paperdoll/socket-системы снаряжения — она только у
  playable-персонажа. Оружие и любые атрибуты рисуются сразу как часть
  спрайта.
  - Исключение: **Sand Naga** (см. ниже) — переключение melee/ranged как
    два независимых набора анимаций, а не сокеты.
- `walk`-анимация через `animate_character(template_animation_id="walk")` —
  это **не** v3, а template-режим, который строится на уже сгенерированном
  теле персонажа (64×64 уже зафиксирован), так что отдельно за размер можно
  не переживать.

## Требование к видовой анатомии (не отступать)

Каждое существо должно читаться как отдельный от человека вид **по
анатомии**, а не только по цвету/одежде. В промпте `create_character`
явно и подробно прописывать анатомические признаки вида (не додумывая
за автора то, чего нет ниже и нет в GDD — если для конкретного существа
описания недостаточно, остановиться и уточнить у автора, а не
импровизировать). Ниже для каждого врага/босса такие признаки уже
сформулированы (на основе GDD + канона классического фэнтези) — можно
использовать как есть.

**Промпт лучше писать на английском** — в этой сессии выяснилось, что
модель заметно надёжнее следует видовым анатомическим признакам
(«goblin», «not human») на английском, чем при переводе тех же терминов
на русский.

## Пайплайн (обязательные шаги на каждого врага/босса)

1. Взять описание из GDD (раздел 8 «Мобы по биомам» + `assets/config/bosses.json`),
   плюс анатомические признаки из этого документа.
2. `create_character(mode="v3", size=64, view="low top-down", outline="single color black outline", detail="medium detail")`
   с промптом на английском, включающим анатомию + оружие/атрибуты как часть
   спрайта.
3. Скачать все 8 idle-ротаций (сеть до `backblaze.pixellab.ai`/`api.pixellab.ai`
   в этой сессии работает — прямой `curl` по URL из `get_character`, без
   плясок вокруг base64).
4. **Визуальная QA idle** (см. критерии ниже) — открыть картинки инструментом
   просмотра, не полагаться на текстовое `success`. Не проходит — не
   продолжать пайплайн, перегенерировать.
5. `reduce_colors` — палитра **всегда** через `palette_image_base64` от
   `assets/palettes/db32.png` (фиксированная палитра, не auto-detect — это
   важно, чтобы разные направления/анимации гарантированно квантовались в
   одинаковые цвета). Для 64×64 можно объединять в один вызов до ~16 кадров
   (суммарный пиксельный бюджет `reduce_colors`).
6. `animate_character(template_animation_id="walk")` на все 8 направлений
   (по одному вызову, ~8 минут).
7. **Известный дефект, который проверять каждый раз**: направления `east`/
   `west` (иногда и другие) периодически рендерятся с заметно более
   насыщенным/ярким оттенком кожи/меха, чем остальные 6 направлений — это
   не баг `reduce_colors` (палитра фиксирована), а вариативность генерации
   конкретного `animate_character`-джоба. Проверять программно (Pillow
   `getcolors`, сравнить доминирующий не-чёрный цвет каждого направления
   между собой) **до** сборки атласа. При расхождении —
   `delete_animation(direction=...)` только для проблемных направлений и
   `animate_character` заново в тот же `animation_group_id`.
8. `reduce_colors` на walk-кадры теми же батчами/той же палитрой.
9. Собрать финальный атлас `<id>_Idle.png` (64×64 ячейки, 8 колонок, 9 строк:
   row0 = 8 idle-ротаций, rows1-8 = walk по направлениям south/south-east/
   east/north-east/north/north-west/west/south-west) + JSON по схеме
   `SpriteSheetLoader` (см. `forest_goblin_archer_Idle.json` как образец).
10. Прогнать через `SpriteSheetLoader::load()` (реальный C++ загрузчик
    проекта, не имитация) — убедиться, что парсится, `cell_size` 64×64.
11. Сохранить в `assets/characters/<id>/<id>_Idle.{png,json}`.
12. Зафиксировать `character_id`/`animation_group_id`/статус в
    `assets/characters/ASSET_REGISTRY.md` (дописывать, не перезаписывать
    существующие записи).
13. Программно проверить DB32-чистоту (0 пикселей вне палитры, opaque) —
    Pillow, как в существующих записях реестра.

## QA / приёмка (обязательный шаг после КАЖДОЙ генерации)

- Контур: единая толщина, чёрный, без разрывов.
- Палитра: 0 внепалитровых пикселей после `reduce_colors` (Pillow-проверка,
  не только визуально).
- Пропорции/ракурс: low top-down, как у базового персонажа.
- **Видовая узнаваемость**: существо не должно читаться как перекрашенный
  человек — при малейшем сомнении отклонять, даже если палитра/контур чистые.
- Размер: буквально 64×64 холст (Pillow `img.size`), контент не обрезан
  (bbox внутри границ).
- Консистентность направлений: доминирующий цвет по каждому из 8 направлений
  сверять между собой (см. пункт 7 пайплайна).
- **На этой итерации всё ещё не принимать финальное решение о качестве
  самостоятельно** — после собственной визуальной проверки показать превью
  (`SendUserFile` или аналог) и дождаться подтверждения автора, прежде чем
  переходить к следующему существу. Если правка мелкая и однозначно
  укладывается в уже одобренный для этого врага стандарт (например,
  донастройка одного направления с явным дефектом) — можно чинить
  проактивно, не спрашивая на каждый чих, но крупные развилки (анатомия,
  поза, оружие) — всегда на подтверждение.

## Список: обычные враги

### Биом 1 — Лес (Tier 1)

**forest_wolf** — «Волк», ближний, быстрый рывок.
- `body_type="quadruped"`, `template="dog"` (ближайший доступный шаблон —
  готовых волков в PixelLab нет; в промпте явно компенсировать словом
  "wolf", а не "dog": худое тело, острая морда, стоячие уши, оскал, тощие
  бока с рёбрами навылет — не домашняя собака).
- Anatomy: lean grey-brown wild wolf, ribs slightly visible, sharp muzzle
  with bared fangs, upright pointed ears, bristled fur along the spine,
  low aggressive stance, no gear (pure animal, no equipment).

**forest_ant** — «Энт (древесный страж)», ближний танк, бьёт по области.
- Это НЕ насекомое-муравей несмотря на английское название id — по GDD это
  Ent (древесный страж). `body_type="humanoid"`.
- Anatomy: living tree guardian, bark-textured limbs fused from gnarled
  roots and branches, no human facial features — a knotted hollow in the
  trunk-like head with two glowing amber eyes, moss and small leaves
  growing across the shoulders, thick root-like feet instead of hands/feet,
  massive slow-moving frame. No held weapon — the body itself is the weapon
  (area-slam attack).

**forest_wasp_swarm** — «Рой ос», дальний, много маленьких юнитов вокруг
игрока.
- **Не укладывается в `create_character`** (гуманоид/quadruped) — это
  насекомое и гейм-дизайн явно про множество мелких копий одного спрайта
  (`SWARM` behavior). Перед генерацией уточнить у автора: делать ли это
  как object-спрайт через `create_8_direction_object`/аналог (не
  character-пайплайн), и не превратится ли это в отдельную визуальную
  задачу (несколько крошечных летающих спрайтов вместо одной боевой
  единицы). Не генерировать вслепую до подтверждения подхода.

### Биом 2 — Пустыня (Tier 2)

**desert_scorpion** — «Скорпион», ближний, яд/DoT.
- Восьминогое членистоногое с клешнями и хвостом-жалом — тоже не ложится
  чисто на humanoid/quadruped шаблоны PixelLab (`bear/cat/dog/horse/lion`).
  Как и с осами — уточнить подход у автора (вероятно `create_object`-путь
  или сильно стилизованный quadruped с явным указанием "giant scorpion,
  not a mammal, chitinous exoskeleton, curled venomous tail stinger,
  pincer claws" и принятием риска, что quadruped-скелет плохо ляжет на
  8-ногое тело). Не тратить генерации до решения.

**desert_raider** — «Наездник на ящерице», ближний+мобильный, таран,
`element: FIRE`.
- Всадник верхом на ящерице — составная композиция (гуманоид + маунт).
  `create_character` не рассчитан на "наездника" из коробки. Варианты:
  (а) сгенерировать как единое существо целиком через явный промпт "a
  reptilian raider fused low to a running desert lizard mount, both as one
  creature silhouette" и посмотреть, справится ли v3-режим; (б) если не
  получается — уточнить у автора, разбивать ли на два отдельных спрайта
  (наездник + ящерица) с ручной композицией в движке. Начинать с (а),
  при провале — остановиться и спросить, не тратить много попыток вслепую.
- Anatomy (для варианта а): sun-scorched desert raider, wrapped in
  sand-worn cloth and leather straps, riding low and fused to a
  fast-moving scaled desert lizard mount with a spiked tail, warm orange/
  fire-tinged skin and gear reflecting the FIRE element, curved
  short blade or spear held ready to charge.

**desert_mummy** — «Мумия», ближний танк, взрывается бинтами при смерти.
- `body_type="humanoid"`.
- Anatomy: desiccated undead mummy, withered bandaged body with strips of
  ancient cloth wrapping visible dry grey-brown skin, hollow sunken eye
  sockets (no visible eyes, just dark voids), stiff shambling posture,
  one arm partially unwrapped showing bone, no held weapon (pure grapple/
  slam attacker per GDD).

**desert_sand_spirit** — «Песчаный дух», дальний, бросает песок,
ослепляет.
- `body_type="humanoid"`.
- Anatomy: ethereal spirit made of swirling compacted sand, no solid legs
  — the lower body dissolves into a loose sand-vortex instead of feet,
  featureless smooth sand-colored head with two glowing pale eyes, tattered
  windswept robe-like sand tendrils trailing from the arms, hands cupped
  as if hurling sand.

### Биом 3 — Зима (Tier 3)

**winter_ice_goblin** — «Ледяной гоблин», ближний, хрупкий, атакует
группами.
- Та же гоблинская анатомия, что у `forest_goblin_archer` (заострённые уши,
  вытянутая нижняя челюсть/клыки, сутулость, крупные кисти/ступни), но
  ледяная тема вместо лесной.
- Anatomy: pale blue-white frost goblin, same inhuman goblin anatomy as
  the forest goblin (pointed ears, elongated fanged jaw, hunched posture,
  oversized clawed hands/feet) but with frost-crusted bluish skin, small
  ice-shard spikes along the shoulders/back, gripping a crude jagged ice
  shortsword or club in one hand as part of the sprite (no ranged weapon —
  this one is melee).

**winter_yeti** — «Йети», ближний танк, очень медленный, огромный урон
по области.
- `body_type="humanoid"` (крупный, но с явно нечеловеческими пропорциями)
  или экспериментально `quadruped template="bear"` с явным описанием
  прямохождения — начать с humanoid.
- Anatomy: massive white/pale-blue furred ape-like beast, long
  disproportionately powerful arms reaching past the knees, huge clawed
  fists, small deep-set eyes, protruding lower tusks, hunched gorilla-like
  posture, breath visible as cold mist, no equipment (pure brute).

**winter_snow_witch** — «Снежная ведьма», дальний, ледяные снаряды,
замедляет, `element: ICE`, сама механика напоминает финального босса.
- `body_type="humanoid"`.
- Anatomy: gaunt frost witch, unnaturally pale cracked-ice skin, hollow
  cheeks, tattered flowing robes of frozen blue-white fabric with icicle
  fringes, hair frozen into jagged ice spikes, glowing cold cyan eyes,
  gripping a twisted ice-crystal staff/wand as part of the sprite.

**winter_ice_spirit** — «Ледяной дух», по GDD дальний/невидим до атаки/
рикошет, но в актуальном конфиге (`enemies.json`) сейчас ближний
(`CHASE`, `isRanged: false`) — **расхождение с GDD, уточнить у автора
поведение перед тем, как закладывать дальнюю атаку в дизайн спрайта**
(если это меняет только руки/позу, не анатомию — не критично, но лучше
свериться).
- `body_type="humanoid"`.
- Anatomy: translucent ice-elemental spirit, semi-transparent icy-blue
  crystalline body with visible facets like fractured glass, no solid
  facial features beyond a faint glowing core where a face would be,
  jagged shard-like protrusions along the limbs instead of hands, faint
  cold mist trailing the silhouette.

### Биом 4 — Мёртвые земли (Tier 4)

**deadlands_skeleton** — «Скелет-воин» по GDD, ближний, возрождается один
раз если не уничтожить кости.
- `body_type="humanoid"`.
- Anatomy: bare bone skeletal warrior, cracked yellowed bones, faint dim
  glow in the empty eye sockets, wielding a notched rusted sword and a
  cracked wooden shield as part of the sprite, held together by scraps of
  rotted leather straps at the joints.

**deadlands_necromancer** — по GDD-таблице этот слот назван «Скелет-лучник»
(дальний, отравленные стрелы), но в конфиге он `deadlands_necromancer` с
`element: DECAY` — **это расхождение между текстом GDD и уже принятым в
проекте id/дизайном, не сглаживать самостоятельно.** Уточнить у автора,
что верно: (а) скелет-лучник с луком и отравленными стрелами, просто
неудачно названный в конфиге "necromancer", или (б) полноценный
некромант-спеллкастер (посох, тёмная магия), и тогда GDD-строка устарела.
От ответа зависит, что держит существо в руке — лук или посох.
- Anatomy (общее для обоих вариантов): gaunt hooded undead spellcaster/
  archer, tattered dark ragged robes, skeletal hands visible at the
  sleeves, sickly green-grey rotting/decayed skin patches consistent with
  DECAY element, hollow glowing dim-purple eyes under the hood.

**deadlands_ghost** — «Призрак», по GDD дальний/мобильный/сквозь стены,
в конфиге сейчас ближний (`CHASE`) — тот же тип расхождения, что у
ice_spirit, уточнить у автора перед финальной позой/руками.
- `body_type="humanoid"`.
- Anatomy: translucent spectral humanoid, semi-transparent pale-blue/grey
  form fading to mist below the waist (no visible legs/feet), tattered
  ethereal robe remnants drifting as if underwater, hollow featureless
  face with only faint glowing eye sockets, elongated ghostly reaching
  hands (touch-damage attacker, no weapon).

**deadlands_bone_golem** — «Костяной голем», ближний танк, при смерти
разлетается на скелетов-воинов.
- `body_type="humanoid"`.
- Anatomy: hulking construct fused from dozens of mismatched bones and
  skulls, asymmetric bone plating across a massive torso, a faint glowing
  rune-marked skull embedded in the chest as a visible core, oversized
  bone-club fists, no true face — just a crude skull-shaped head on top.

## Список: боссы (`assets/config/bosses.json`)

Боссы визуально должны читаться заметно крупнее и опаснее обычных врагов
того же биома (это уже частично задано `size` в конфиге — держать это в
голове при `create_character size=` , можно смело брать 96–128 вместо 64,
раз v3-режим передаёт размер буквально). GDD не даёт для боссов вообще
никакого текстового описания кроме `id`/`theme` в конфиге — анатомия ниже
составлена по названию/теме; **при генерации боссов явно подтвердить у
автора общее направление до первой генерации**, это не мелкая правка, а
ключевой арт для игры.

**elder_druid** (тема: лес, `size` 64×64 в конфиге, но можно крупнее для
босса).
- Anatomy: ancient corrupted druid, humanoid but visibly consumed by the
  forest — bark has grown over parts of the skin, twisted antlers growing
  from the skull, thick vines and small roots emerging from the arms and
  back instead of normal limbs texture, glowing sickly green eyes, a
  gnarled living-wood staff fused into one hand as part of the sprite,
  moss-covered tattered druidic robes.

**sand_naga** (тема: пустыня — **Sand Naga = исключение из правила "без
переключения снаряжения"**, см. Технический стандарт: делать два полных
независимых набора анимаций, melee и ranged, переключаемых в коде как
разные state).
- Anatomy: serpentine naga, a humanoid torso fused seamlessly into a long
  scaled sand-colored snake tail instead of legs, cobra-like hood flares
  at the neck, reptilian slit-pupil eyes, forked tongue, sun-bleached
  golden-tan scales.
  - Melee state: dual curved venomous fangs/claws bared, coiled striking
    posture, no ranged weapon visible.
  - Ranged state: a separate full sprite set where the naga instead holds/
    spits sand-crystal projectiles or a bone-tipped throwing spear — needs
    explicit confirmation from the author on the exact ranged attack prop
    before generating, since GDD gives no detail beyond "melee/ranged".

**frost_elemental** (тема: зима).
- Не гуманоид и не типичное животное — стихийное существо. Начать с
  `body_type="humanoid"` с явным "not a person, an elemental being" в
  промпте; если результат читается слишком человечно (тот же провал, что
  был у гоблина попытка 2/3), рассмотреть генерацию как object/tile-подобный
  стихийный сгусток льда вместо character-пайплайна — уточнить у автора,
  если стандартный character-подход не даёт убедительного результата за
  1-2 попытки.
- Anatomy: a living mass of jagged ice and frost, roughly humanoid-shaped
  but with no true face — just a bright glowing cyan-white core visible
  through cracks in an irregular crystalline "body", sharp icicle shards
  jutting asymmetrically from the shoulders and forearms instead of hands,
  no clothing/gear (the ice itself is both body and armor), trailing cold
  mist/frost particles.

**ancient_lich** (тема: мёртвые земли).
- Anatomy: a regal undead lich, skeletal frame wrapped in tattered once-
  ornate dark purple and black royal/arcane robes with faded gold trim, a
  cracked ancient crown or jeweled circlet on the bare skull, glowing
  violet eye sockets, gripping a tall ornate arcane staff topped with a
  glowing dark crystal as part of the sprite, a faint dark purple aura/
  mist trailing the robes.

## Формат отчёта (как раньше)

По каждому врагу/боссу:
- `character_id`, `animation_group_id` (если применимо);
- какие шаги пайплайна пройдены/не пройдены;
- результат визуальной QA-проверки по каждому шагу (принято / расхождения);
- для позиций, помеченных «уточнить у автора» — явно остановиться и
  задать вопрос, прежде чем генерировать, а не импровизировать.

## Известные риски / не наступать повторно (дополнено по итогам первой
итерации)

- `standard`-режим `create_character` может отдать холст больше запрошенного
  `size` (авто-расширение) — использовать `v3`.
- Анатомия "просто цвет" не проходит QA — нужны явные видовые признаки в
  промпте, на английском.
- Оружие в руке нужно формулировать явно как "gripping ... firmly in one
  hand, held out and clearly visible" — расплывчатое "drawing a bow"
  может не отрисоваться видимым образом.
- После `reduce_colors` обязательно сверять доминирующий цвет по всем 8
  направлениям друг с другом — East/West регулярно "уезжают" по
  насыщенности даже при фиксированной палитре (это генерация, не
  квантование), чинится точечной перегенерацией только этих направлений.
- `reduce_colors` имеет лимит суммарных пикселей за вызов — при 64×64
  можно смело батчить ~12-16 кадров, при более крупных холстах меньше.
- `animate_character`-джобы и `reduce_colors`-джобы истекают (после ~8 часов
  результат недоступен по `job_id`) — скачивать сразу после готовности, не
  откладывать на потом.
