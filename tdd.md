# Technical Design Document
## The Circle — текущая реализация

Этот документ описывает **фактическое устройство кода** (на момент последнего
обновления). В отличие от `gdd.md` (концепция/дизайн), здесь фиксируется, как
система реализована: архитектура, компоненты ECS, формулы, порядок обновления
систем и форматы конфигов. Обновляйте этот файл при значимых изменениях
архитектуры или баланса.

---

## 1. Стек технологий

| Слой | Технология |
|------|-----------|
| Язык | C++17 |
| Графика/окно/звук/ввод | SFML 2.6.x |
| ECS | EnTT 3.13.x (`entt::registry`) |
| Конфиги | nlohmann/json |
| Сборка | CMake + Conan + Ninja, MSVC (VS 2022), пресет `conan-release` |
| Точка входа | `src/main.cpp` → `tc::Game` |
| Скрипты | `build.ps1` (сборка), `setup.ps1` (установка зависимостей) |

Логическое разрешение игры зафиксировано в `Game::LOGICAL_WIDTH/HEIGHT = 1280x720`
(letterbox-вид через `sf::View`, см. `Game::applyLetterboxView`).

---

## 2. Архитектура приложения

`Game` (`src/Game.hpp/.cpp`) — корневой объект:
- владеет `sf::RenderWindow` и `sf::View` (леттербокс 1280×720);
- владеет стеком состояний `std::stack<std::unique_ptr<IGameState>>`;
- владеет сквозными сервисами: `Localization`, `AudioManager`, `MetaProgression`, `FontManager`.

Переходы между состояниями (`pushState` / `popState` / `changeState`) **откладываются**
(`PendingAction`) и применяются в начале следующего кадра — безопасно вызывать
из `update()`/`handleInput()` состояния.

Каждый кадр (`Game::run`):
1. `processInput()` — рассылает `sf::Event` активному состоянию;
2. `applyPendingStateChange()`;
3. `update(dt)` — делегирует в `state->update(dt)`;
4. `render()` — `window.clear()`, `state->render(window)`, `window.display()`.

### Игровые состояния (`src/states/`)

| Состояние | Назначение |
|-----------|-----------|
| `MainMenuState` | Главное меню: New Game / Quit / переключение языка RU↔EN |
| `PlayState` | Основной игровой цикл одного рана (ECS-мир, биомы, бой) |
| `DeathState` | Экран окончания рана (победа/смерть), показывает заработанные meta-очки, ведёт в `MetaUpgradeState` |
| `MetaUpgradeState` | Экран трат meta-очков на Strength/Endurance/Health, ведёт в `MainMenuState` |

---

## 3. Игровой цикл `PlayState`

`PlayState` владеет `entt::registry registry`, `World world`, ссылкой на игрока
`entt::entity player` и набором систем-членов (порядок объявления в
`PlayState.hpp` соответствует порядку вызова в `update()`):

```
movementSystem      MovementSystem
collisionSystem     CollisionSystem
aiSystem            AISystem
combatSystem        CombatSystem
statusEffectSystem  StatusEffectSystem
lootSystem          LootSystem
renderSystem        RenderSystem
```

### Порядок обновления за кадр (`PlayState::update`)

1. Читаем ввод (WASD/стрелки) → `Velocity` и `Facing` игрока.
2. `MovementSystem::update` — интегрирует `Position += Velocity * Speed * dt`
   (с учётом замедления `StatusEffect::SLOW`).
3. `CollisionSystem::update` — раздвигает пересекающиеся `Position+Renderable+Health`
   сущности (кроме `PhaseThrough`).
4. **Clamp игрока** к границам мира (`Game::LOGICAL_WIDTH/HEIGHT`).
5. **Clamp всех `EnemyTag` (включая боссов) к границам мира** — новая правка,
   см. раздел 8 «Границы мира».
6. `AISystem::update` — поведение врагов/боссов, включая фазы боссов и
   спавн миньонов Лича.
7. `CombatSystem::update` — атаки (melee/ranged), снаряды, урон.
8. `StatusEffectSystem::update` — тики яда/слоу, истечение эффектов.
9. `LootSystem::update` — удаляет мёртвых, дроп фрагментов/экипировки, обновляет `RunSummary`.
10. Обновление списка врагов текущего биома; если волна пуста — `spawnBiomeEnemies()`.
11. Проверка `Health` игрока ≤ 0 → `finishRun(false)`.
12. В комнате босса: если все `BossTag`-сущности мёртвы → `finishRun(true)`.

Рендер (`PlayState::render`): фон по биому → `RenderSystem` → `HUD` → `TutorialHint`.

---

## 4. Мир и прогрессия (`src/world/`)

- `BiomeType { FOREST=0, DESERT=1, WINTER=2, DEADLANDS=3 }`, `BIOME_COUNT = 4`.
- `biomeTier(type) = int(type) + 1` — тир экипировки/брони совпадает с индексом биома (1–4).
- `World` хранит 4 `Biome` + случайно выбранную тему центральной комнаты босса
  (`bossRoomTheme`, роллится в `World::generate()`).
- `Biome`:
  - `KEY_FRAGMENTS_REQUIRED = 5` — фрагментов для разблокировки перехода;
  - хранит `std::vector<entt::entity> enemies` текущей волны и счётчик собранных фрагментов;
  - `isUnlocked()` — true, когда `keyFragmentsCollected >= 5`.
- Переход (`PlayState::advanceToNextBiome`, клавиша **F**, доступна когда биом разблокирован):
  - очищает врагов текущего биома, `runSummary.biomesCleared++`;
  - если текущий биом — DEADLANDS → `enterBossRoom()` (спавн `BossTemplate` по теме `bossRoomTheme`
    в центре арены, музыка `"boss"`, `inBossRoom = true`);
  - иначе переключается на следующий `BiomeType`, спавнит врагов, меняет музыку.

---

## 5. ECS: компоненты (`src/ecs/Components.hpp`)

| Компонент | Поля | Назначение |
|-----------|------|-----------|
| `Position` | x, y | Мировые координаты |
| `Velocity` | dx, dy (нормализовано) | Направление движения |
| `Facing` | dx, dy | Последнее ненулевое направление — для офсета melee-атаки |
| `Health` | current, max | Здоровье |
| `Armor` | `value` (int, плоское снижение), `damageReductionPercent` (float, % снижения от Endurance) | Снижение урона |
| `Damage` | value | Урон атаки/снаряда |
| `Speed` | value | Скорость, ед/сек |
| `PlayerTag` | — | Маркер игрока |
| `EnemyTag` | `biome` (1–4, = tier) | Маркер врага/босса + его тир |
| `Name` | `id` | Ключ локализации `enemy.<id>` для неймплейта |
| `ProjectileTag` | `ownerBiome` (0 = игрок) | Снаряд |
| `Renderable` | color, size | Заглушка-прямоугольник для рендера |
| `MeleeCombat` | range, cooldown, timer, offset | Ближний бой; offset — смещение точки атаки вдоль `Facing` (только игрок) |
| `RangedCombat` | range, projectileSpeed, cooldown, timer | Дальний бой — спавнит `ProjectileTag` |
| `BlockAbility` | reductionPercent, isBlocking | % снижения урона при блоке (меч+щит) |
| `Potion` | healAmount, charges, maxCharges, killsPerCharge, killCounter | Зелье на зарядах (PoE-style) |
| `HitFlash` | timer (`DURATION = 0.15f`) | Кратковременная красная вспышка при попадании |
| `StatusEffect` | type (`POISON`/`SLOW`), dps, duration, timer, damageAccumulator | Яд (DoT) или замедление |
| `KeyFragmentDrop` | chance | Шанс дропа фрагмента ключа |
| `EquipmentDrop` | chance | Шанс дропа экипировки |
| `BossTag` | — | Маркер босса центральной комнаты |
| `KeyFragmentHolder` | count | Счётчик фрагментов у игрока |
| `ReviveOnce` | used | Враг воскресает один раз с половиной HP |
| `PhaseThrough` | — | Игнорирует `CollisionSystem` (призраки) |
| **`Invulnerable`** | — | **Новое.** Сущность не получает урон ни из какого источника (используется для фазы щита Лича) |
| `AIBehavior` | type: `CHASE`, `RANGED_KEEP_DISTANCE`, `SWARM`, `BOSS_DRUID`, `BOSS_NAGA`, `BOSS_ELEMENTAL`, `BOSS_LICH` | Тип поведения ИИ |
| `BossAI` | phaseTimer, phaseIndex, baseSpeed, rangedMode, **`nextSummonThreshold`**, **`summonedMinions` (`vector<entt::entity>`)** | Состояние фаз боссов; два последних поля — новая механика Лича (см. раздел 8) |
| `Equipment` | helmetTier, chestTier, leggingsTier, weaponTier, weaponType (`NONE/SWORD/BOW`) | Экипировка игрока (0 = нет) |
| `MetaStats` | strength, endurance, health, points | Meta-прогрессия, персистится в `meta_save.json` |

---

## 6. ECS: системы (`src/ecs/systems/`)

| Система | Файл | Описание |
|---------|------|----------|
| `MovementSystem` | Movement* | `Position += Velocity * Speed * dt`; скорость ×0.5 при `StatusEffect::SLOW` |
| `CollisionSystem` | Collision* | Разводит пересекающиеся AABB (`Position+Renderable+Health`, без `PhaseThrough`) по оси наименьшего перекрытия, по 50% смещения на каждую сторону |
| `AISystem` | AISystem* | Считает направление к игроку, применяет поведение (`AIBehavior`), обновляет фазы боссов (`BossAI`); спавнит миньонов Лича (см. раздел 8) |
| `CombatSystem` | Combat* | Melee/ranged атаки, снаряды, `applyDamage` (формула — раздел 7), заряды зелья при килле |
| `StatusEffectSystem` | StatusEffect* | Тик яда (если не `Invulnerable`), уменьшение таймера, удаление истёкших `StatusEffect` |
| `LootSystem` | Loot* | Убирает мёртвых (`ReviveOnce` — воскрешение), роллит `KeyFragmentDrop`/`EquipmentDrop`, гарантированный дроп при очистке волны |
| `RenderSystem` | Render* | Рисует `Renderable`, оверлей щита для `Invulnerable` (новое, раздел 8), HP-бар и неймплейт для `EnemyTag+Health+Name` |

### Безопасность итерации EnTT

`AISystem` создаёт новых врагов (миньонов Лича) через `PendingSpawn`-буфер и
применяет их **после** основного цикла `for (auto entity : view)`, чтобы не
инвалидировать итераторы пулов `EnemyTag/AIBehavior/Position/Velocity` во время
итерации. Добавление/удаление компонентов **другого** типа (например,
`Invulnerable`) к уже существующим сущностям внутри цикла безопасно.

---

## 7. Боевая система и формула урона

`CombatSystem::applyDamage(attacker, target, rawDamage)`:

```text
if target has Invulnerable -> урон полностью игнорируется (return)

armor    = target.Armor.value (если есть), иначе 0
reduction = target.Armor.damageReductionPercent (если есть)
if target.BlockAbility.isBlocking:
    reduction += BlockAbility.reductionPercent
reduction = min(reduction, MAX_DAMAGE_REDUCTION = 0.9)

mitigated = (rawDamage - armor) * (1 - reduction)
mitigated = max(mitigated, 1)   // минимум 1 урона, если атака не заблокирована Invulnerable

health.current -= mitigated
-> HitFlash на target
-> если health.current == 0 и у attacker есть Potion: killCounter++, при достижении killsPerCharge — +1 заряд (до maxCharges)
```

- **Melee** (`updateMelee`): у игрока — атака по area (бьёт **всех** врагов в
  радиусе `range` от точки `Position + Facing * offset`); у врагов — только
  ближайшая цель в радиусе `range` (центр сущности).
- **Ranged** (`updateRanged`): на кулдауне выбирает ближайшую цель в `range`,
  спавнит `ProjectileTag` (`EntityFactory::createProjectile`).
- **Projectiles** (`updateProjectiles`): уничтожаются за границами мира
  (`WORLD_WIDTH/HEIGHT = 1280×720`, локальные константы в `CombatSystem.cpp`)
  или при попадании (`PROJECTILE_HIT_RADIUS = 20`) во врага/игрока (в
  зависимости от `ownerBiome`).
- Статус-эффект атакующего (`StatusEffect`, если есть) переносится на цель при
  попадании melee.

---

## 8. Боссы центральной комнаты (`assets/config/bosses.json`)

Все боссы создаются через `EntityFactory::createBoss`: получают `BossTag`,
`BossAI{phaseTimer=0, phaseIndex=0, baseSpeed=tmpl.speed, rangedMode=false}`,
и боевой компонент — `RangedCombat` для `BOSS_ELEMENTAL`, иначе `MeleeCombat`
(range = max(size.x, size.y), cooldown = 1.0).

| Boss id | Тема | HP | Урон | Броня | Speed | Поведение |
|---------|------|----|------|-------|-------|-----------|
| `elder_druid` | forest | 600 | 18 | 6 | 70 | `BOSS_DRUID` |
| `sand_naga` | desert | 700 | 22 | 8 | 65 | `BOSS_NAGA` |
| `frost_elemental` | winter | 750 | 25 | 10 | 55 | `BOSS_ELEMENTAL` |
| `ancient_lich` | deadlands | 850 | 30 | 8 | 60 | `BOSS_LICH` |

### BOSS_DRUID (Elder Druid)
Каждые `DRUID_PHASE_DURATION = 3.0s` циклически меняет фазу (`phaseIndex % 3`),
меняя множитель скорости и `MeleeCombat.range`:
- Bear (0): ×0.6 скорости, range 90 (медленный, широкий удар);
- Hawk (1): ×1.8 скорости, range 35 (быстрый, короткий удар);
- Fox (2): ×1.1 скорости, range 55 (баланс).
Движение — постоянное преследование игрока (`CHASE`-логика).

### BOSS_NAGA (Sand Naga)
Каждые `NAGA_PHASE_DURATION = 4.0s` переключает `rangedMode`:
- ranged-фаза: снимает `MeleeCombat`, ставит `RangedCombat{range=260, projSpeed=320, cooldown=1.2}`,
  движение — держит дистанцию (`applyKeepDistance`, целевая дистанция = `range*0.8`);
- melee-фаза: снимает `RangedCombat`, ставит `MeleeCombat{range=max(size), cooldown=1.0}`,
  движение — преследование.

### BOSS_ELEMENTAL (Frost Elemental)
`RangedCombat{range=260, projSpeed=320, cooldown=1.2}` с самого начала.
Каждые `ELEMENTAL_SLOW_INTERVAL = 4.0s` накладывает на игрока
`StatusEffect::SLOW` длительностью `ELEMENTAL_SLOW_DURATION = 2.0s`.
Движение — всегда держит дистанцию (`applyKeepDistance`, целевая дистанция = `range*0.8`).

### BOSS_LICH (Ancient Lich) — обновлённая механика

Раньше Лич был полностью статичен (нулевая скорость, периодический спавн
миньонов по таймеру, без боевого компонента). Текущая реализация:

**Движение.** Лич преследует игрока как обычный `CHASE`-моб (использует
`MeleeCombat{range=72, cooldown=1.0}`, выставленный в `EntityFactory::createBoss`).

**Фазы щита по HP-порогам** (`AISystem.cpp`, `LICH_SUMMON_THRESHOLDS = {0.75, 0.50, 0.25}`):

1. Каждый кадр, пока Лич **не** под щитом, проверяется
   `health.current / health.max <= LICH_SUMMON_THRESHOLDS[nextSummonThreshold]`.
2. При срабатывании:
   - `nextSummonThreshold++` (индекс следующего порога: 75% → 50% → 25%, максимум 3 срабатывания за бой);
   - Личу добавляется компонент `Invulnerable` — он становится **неуязвимым**
     (любой `applyDamage` по нему игнорируется, тик яда от `StatusEffectSystem` тоже пропускается);
   - спавнится волна из **2–3 случайных врагов биома Deadlands**
     (`ConfigLoader::getEnemyConfig().getEnemiesForBiome(DEADLANDS)`), позиции —
     случайный офсет ±80 от позиции босса, кламп к границам мира с отступом 40px;
   - все созданные сущности записываются в `BossAI.summonedMinions`.
3. Пока Лич под щитом, каждый кадр проверяется, остался ли хоть один живой
   (`registry.valid(...)`) миньон из `summonedMinions`.
   - Если **все миньоны мёртвы** → `Invulnerable` снимается, `summonedMinions`
     очищается, Лич снова получает урон.
4. Если игрок наносит урон, опускающий HP сразу за несколько порогов (например,
   с 100% до 20% за один удар), фазы срабатывают **последовательно**: щит
   поднимается → миньоны убиты → щит снова поднимается на следующем пороге
   → и так далее, пока `nextSummonThreshold` не достигнет 3.

**Визуализация щита** (`RenderSystem.cpp`): любая сущность с `Invulnerable`
получает оверлей — полупрозрачный ярко-фиолетовый прямоугольник
(`fillColor = RGBA(180,70,255,90)`, `outlineColor = RGBA(220,140,255,220)`,
`outlineThickness = 2`), размером `Renderable.size + (16,16)`, отрисовываемый
сразу после основного спрайта боксa.

---

## 9. Meta-прогрессия (`src/meta/MetaProgression.*`)

Очки начисляются по итогам рана (`computePointsEarned`):

| Источник | Очки |
|----------|------|
| `POINTS_PER_10_KILLS = 1` | за каждые 10 убийств |
| `POINTS_PER_BIOME = 2` | за каждый пройденный биом |
| `POINTS_FOR_BOSS = 5` | за победу над финальным боссом |
| `POINTS_PER_FRAGMENT = 0` | за каждый собранный фрагмент (выключено, легко включить) |

Очки тратятся 1:1 на уровни `Strength` / `Endurance` / `Health`
(`spendPointOn*`, экран `MetaUpgradeScreen`). Сохраняются в `meta_save.json`
(в `.gitignore`).

### Эффекты — процентные, не абсолютные

Раньше каждый уровень давал фиксированный бонус (+10 HP, +1 урона, +1 брони).
Теперь каждый уровень даёт **% от итогового значения**, применяемый как
множитель **после** учёта экипировки — так бонусы остаются значимыми на любом
тире снаряжения:

| Стат | Константа (`MetaProgression.hpp`) | Эффект |
|------|-----------------------------------|--------|
| Strength | `DAMAGE_PERCENT_PER_STRENGTH = 0.02` | `Damage = round(baseWeaponDamage * (1 + strength * 0.02))` — т.е. +2% урона за уровень |
| Endurance | `DAMAGE_REDUCTION_PERCENT_PER_ENDURANCE = 0.02` | `Armor.damageReductionPercent = endurance * 0.02` — +2% снижения урона за уровень (суммируется с блоком, капается на 90% в `CombatSystem`) |
| Health | `HEALTH_PERCENT_PER_LEVEL = 0.02` | `Health.max = round((baseHealth + equipmentHealthBonus) * (1 + health * 0.02))` — +2% максимального HP за уровень |

Применяется в `EntityFactory::applyEquipmentStats`, вызываемом при создании
игрока и при каждом дропе экипировки.

---

## 10. Лут и экипировка

- `LootSystem::update` каждый кадр проверяет `EnemyTag+Health` с `current <= 0`.
  - `ReviveOnce` (не использован) → воскрешает с `max/2` HP, не уничтожается.
  - Иначе — кандидат на уничтожение.
- **Гарантированный дроп**: если уничтожение текущих кандидатов опустошает
  `waveEnemies` биома, последняя обработанная сущность гарантированно роняет
  фрагмент ключа и экипировку (минуя ролл шанса).
- `KeyFragmentDrop.chance` → `KeyFragmentHolder.count++`, `RunSummary.keyFragments++`.
- `EquipmentDrop.chance` → выбирает слот (0=шлем, 1=кираса, 2=поножи, 3=оружие)
  среди ещё не выпавших на этом тире (`droppedSlots[tier-1][slot]`), тир = `EnemyTag.biome`.
  Оружие по умолчанию — `SWORD`, если ещё не задано.
- После дропа экипировки `PlayState` вызывает `EntityFactory::applyEquipmentStats`
  (пересчёт армора/HP/урона/скорости/боевых компонентов) и `HUD::showEquipmentDrop`.

### Тиры экипировки (`assets/config/equipment.json`)

| Tier | Шлем (armor/HP) | Кираса (armor/HP) | Поножи (armor/HP/speed) | Меч (dmg/atkSpd/block/range) | Лук (dmg/atkSpd/range) |
|------|-----------------|--------------------|--------------------------|-------------------------------|--------------------------|
| 1 | 1 / 5 | 2 / 10 | 1 / 5 / 5 | 10 / 1.5 / 0.20 / 45 | 8 / 1.0 / 280 |
| 2 | 2 / 10 | 4 / 20 | 2 / 10 / 10 | 15 / 1.6 / 0.25 / 48 | 12 / 1.1 / 300 |
| 3 | 3 / 15 | 6 / 30 | 3 / 15 / 15 | 22 / 1.7 / 0.30 / 50 | 18 / 1.2 / 320 |
| 4 | 4 / 20 | 8 / 40 | 4 / 20 / 20 | 30 / 1.8 / 0.35 / 52 | 25 / 1.3 / 340 |

---

## 11. Границы мира и коллизии

Константы мира `WORLD_WIDTH/HEIGHT = 1280×720` совпадают с
`Game::LOGICAL_WIDTH/HEIGHT` и используются в `AISystem` (спавн миньонов) и
`CombatSystem` (уничтожение снарядов за границей).

`PlayState::update`, после `MovementSystem` + `CollisionSystem`:

1. Игрок клампится к `[size/2, LOGICAL_WIDTH - size/2] × [size/2, LOGICAL_HEIGHT - size/2]`
   (было и раньше).
2. **Новое:** все сущности с `EnemyTag+Position+Renderable` (включая боссов)
   клампятся к тем же границам. Это устраняет уход врагов за пределы экрана,
   который раньше мог возникать из-за раздвижки `CollisionSystem` у края арены
   или из-за спавна миньонов босса со случайным офсетом.

`CollisionSystem` сама по себе не знает о границах мира — раздвигает только
взаимно пересекающиеся сущности; ограничение экрана — отдельный шаг в
`PlayState::update`.

---

## 12. Конфигурационные файлы (`assets/config/`, через `ConfigLoader`)

`ConfigLoader::get().loadAll("assets/config")` — синглтон, загружает:

| Файл | Структура | Назначение |
|------|-----------|-----------|
| `player.json` | `PlayerConfig` | Базовые статы игрока (скорость, HP, melee/ranged параметры, зелье) |
| `equipment.json` | `EquipmentConfig` | Массивы `ArmorPieceStats`/`WeaponStats` по тирам 1–4 для helmet/chest/leggings/sword/bow |
| `enemies.json` | `EnemyConfig.enemies` (`vector<EnemyTemplate>`) | Обычные враги по биомам (id, biome, health, damage, armor, speed, behavior, ranged/melee параметры, статус-эффекты, флаги revivesOnce/phaseThrough, шансы дропа, цвет/размер) |
| `bosses.json` | `EnemyConfig.bosses` (`vector<BossTemplate>`) | 4 босса центральной комнаты (id, theme, health/damage/armor/speed, behavior, цвет/размер) |

`EnemyConfig::getEnemiesForBiome(biome)` — фильтр по `BiomeType`;
`getBossForTheme(theme)` — поиск босса по теме комнаты.

---

## 13. UI, локализация, аудио

- **HUD** (`src/ui/HUD.*`): здоровье, счётчик фрагментов ключа
  (`x / KEY_FRAGMENTS_REQUIRED`), заряды зелья, строка экипировки
  (`Sword T0 | Helmet T0 | Chest T0 | Legs T0`), подсказка "следующий биом
  открыт" (F), временное уведомление о найденной экипировке. Тексты
  кешируются (`textInitialized`) и обновляются только при изменении значений.
- **RenderSystem**: прямоугольники-заглушки с цветом из `Renderable`,
  покраснение при `HitFlash`, фиолетовый оверлей щита при `Invulnerable`,
  HP-бар + неймплейт (`Name` → `enemy.<id>`) для врагов/боссов.
- **TutorialHint** (`src/ui/TutorialHint.*`): однократные обучающие подсказки
  с обработкой кликов закрытия.
- **Localization** (`src/core/Localization.*`, `assets/lang/{ru,en}.json`):
  ключи вида `enemy.<id>`, `meta.*`, UI-строки; переключение языка в главном меню.
- **FontManager**: кириллический и латинский шрифты (`assets/fonts/`),
  выбор по текущему языку.
- **AudioManager**: `playMusic(track)` — `forest`/`desert`/`winter`/`deadlands`/`boss`
  (стриминг `sf::Music`), `playSfx(name)` — короткие эффекты через
  `sf::SoundBuffer`/`sf::Sound`.

---

## 14. Сборка и инструменты

- `setup.ps1` — разворачивает зависимости (Conan).
- `build.ps1` — настраивает окружение MSVC (vcvarsall), запускает
  `cmake --build --preset conan-release` (Ninja), результат —
  `build/TheCircle.exe`.
- `meta_save.json` — runtime-файл сохранения meta-прогрессии в корне репозитория
  (создаётся при первом сохранении, в `.gitignore`).

---

## 15. Известные ограничения и направления развития

- Графика — прямоугольники-заглушки (`Renderable`), спрайты не подключены
  (`assets/textures/` пуст).
- `POINTS_PER_FRAGMENT = 0` — фрагменты ключа сейчас не дают meta-очков напрямую
  (легко изменить в `MetaProgression.hpp`).
- Комната финального босса доступна только после прохождения Deadlands
  (биом 4); тема комнаты выбирается случайно из 4 тем при `World::generate()`.
- Уникальные бонус-статы экипировки (лайфстил, % урона по типу и т.п. из
  `gdd.md`) пока не реализованы — `ArmorPieceStats`/`WeaponStats` содержат
  только базовые поля.
- Чанки/процедурная генерация окружения из `gdd.md` не реализованы — биомы
  это плоские арены 1280×720 со случайным спавном врагов.
