# Technical Design Document
## The Circle — C++ / SFML / EnTT (v8, полная версия)

---

## 1. Стек технологий

| Компонент | Библиотека | Версия |
|-----------|-----------|--------|
| Рендер, окно, ввод, звук | SFML | 2.6.x |
| ECS | EnTT | 3.13.x |
| Конфиги (JSON) | nlohmann/json | 3.11.x |
| Компилятор | MSVC | VS 2022 |
| Сборка | CMake | 3.16+ |
| Стандарт C++ | C++17 | — |

Зависимости управляются через **Conan 2**. Все библиотеки устанавливаются командой `conan install . --build=missing` перед первой сборкой.

Файл `conanfile.txt`:
```ini
[requires]
sfml/2.6.1
entt/3.13.0
nlohmann_json/3.11.3

[generators]
CMakeDeps
CMakeToolchain
```

---

## 2. Файловая структура

```
TheCircle/
├── CMakeLists.txt
├── conanfile.txt
├── assets/
│   ├── config/
│   │   ├── player.json
│   │   ├── equipment.json
│   │   ├── enemies.json
│   │   └── bosses.json
│   ├── fonts/
│   ├── lang/
│   │   ├── ru.json
│   │   └── en.json
│   ├── sounds/
│   │   ├── music/
│   │   └── sfx/
│   └── textures/            # пока пусто — рендер прямоугольниками-заглушками
└── src/
    ├── main.cpp
    ├── Game.hpp / Game.cpp
    ├── core/
    │   ├── AudioManager.hpp/.cpp
    │   ├── FontManager.hpp/.cpp
    │   ├── Localization.hpp/.cpp
    │   └── TextUtils.hpp
    ├── states/
    │   ├── IGameState.hpp
    │   ├── MainMenuState.hpp/.cpp
    │   ├── PlayState.hpp/.cpp
    │   ├── DeathState.hpp/.cpp
    │   └── MetaUpgradeState.hpp/.cpp
    ├── ecs/
    │   ├── Components.hpp
    │   ├── systems/
    │   │   ├── MovementSystem.hpp/.cpp
    │   │   ├── CollisionSystem.hpp/.cpp
    │   │   ├── CombatSystem.hpp/.cpp
    │   │   ├── RenderSystem.hpp/.cpp
    │   │   ├── AISystem.hpp/.cpp
    │   │   ├── StatusEffectSystem.hpp/.cpp
    │   │   └── LootSystem.hpp/.cpp
    │   └── EntityFactory.hpp/.cpp
    ├── world/
    │   ├── World.hpp/.cpp
    │   ├── Biome.hpp/.cpp
    │   └── BiomeType.hpp
    ├── config/
    │   ├── ConfigLoader.hpp/.cpp
    │   ├── PlayerConfig.hpp
    │   ├── EquipmentConfig.hpp   # ArmorPieceStats/WeaponStats, TIER_COUNT = 4
    │   └── EnemyConfig.hpp/.cpp
    ├── meta/
    │   └── MetaProgression.hpp/.cpp
    └── ui/
        ├── HUD.hpp/.cpp
        ├── MetaUpgradeScreen.hpp/.cpp
        ├── TutorialHint.hpp/.cpp
        └── PauseScreen.hpp/.cpp
```

---

## 3. Game Loop и State Machine

### Состояния
| Состояние | Переход |
|-----------|---------|
| `MainMenuState` | → `PlayState` (новый ран) |
| `PlayState` | → `DeathState` (смерть или победа) |
| `DeathState` | → `MetaUpgradeState` |
| `MetaUpgradeState` | → `MainMenuState` |

Смена состояний **отложена** (`pendingAction`) — безопасно вызывать из `update()` и `handleInput()`.

Окно масштабируемое — `sf::View` с логическим разрешением **1280×720**, letterbox при нестандартном соотношении сторон. `applyLetterboxView()` вызывается в конструкторе и при `sf::Event::Resized`.

### Пауза в `PlayState`

Клавиша **Escape** переключает флаг `paused`. Пока `paused == true`:
- `update()` не выполняет ни одной ECS-системы (`lastDt = 0`, мир заморожен);
- `render()` дополнительно рисует `PauseScreen` — полупрозрачный оверлей с
  кнопками **Resume** и **Main Menu**;
- клики мыши обрабатываются через `PauseScreen::getButtonAt()`:
  `Resume` снимает паузу, `Main Menu` сразу переключает на `MainMenuState`
  (без сохранения прогресса текущего рана).

---

## 4. ECS — Компоненты (`ecs/Components.hpp`)

```cpp
struct Position       { float x, y; };
struct Velocity       { float dx, dy; };
struct Facing         { float dx = 0, dy = 1; };  // последнее ненулевое направление

struct Health         { int current, max; };

// value — плоское снижение урона; damageReductionPercent — % снижения
// от Endurance (meta-прогрессия), складывается с BlockAbility и капается на 90%
struct Armor          { int value; float damageReductionPercent = 0.0f; };

struct Damage         { int value; };
struct Speed          { float value; };

struct PlayerTag      {};
struct BossTag        {};
struct EnemyTag       { int biome; };               // 1–4, совпадает с тиром экипировки

// Ключ локализации "enemy.<id>" для неймплейта над HP-баром
struct Name           { std::string id; };

struct ProjectileTag  { int ownerBiome; };          // 0 = снаряд игрока
struct PhaseThrough   {};                            // игнорирует CollisionSystem (призраки)

// Полный иммунитет к урону и тикам яда (фаза щита BOSS_LICH)
struct Invulnerable   {};

struct Renderable     { sf::Color color; sf::Vector2f size; };
struct HitFlash       { static constexpr float DURATION = 0.15f; float timer = DURATION; };

// offset — смещение точки атаки вдоль Facing (используется только игроком)
struct MeleeCombat    { float range; float cooldown; float timer; float offset; };
struct RangedCombat   { float range; float projectileSpeed; float cooldown; float timer; };
struct BlockAbility   { float reductionPercent; bool isBlocking; };

struct Potion         { int healAmount; int charges; int maxCharges;
                        int killsPerCharge; int killCounter; };

struct StatusEffect   { enum Type { POISON, SLOW } type;
                        float dps; float duration; float timer;
                        float damageAccumulator = 0.f; };  // накопитель для дробного DoT

struct KeyFragmentDrop   { float chance; };
struct EquipmentDrop     { float chance = 0.1f; };
struct KeyFragmentHolder { int count; };
struct ReviveOnce        { bool used; };

struct AIBehavior { enum Type { CHASE, RANGED_KEEP_DISTANCE, SWARM,
                                BOSS_DRUID, BOSS_NAGA, BOSS_ELEMENTAL, BOSS_LICH } type; };

// Состояние фаз боссов
struct BossAI {
    float phaseTimer = 0.0f;
    int phaseIndex = 0;
    float baseSpeed = 0.0f;     // базовая скорость до фазовых множителей
    bool rangedMode = false;

    // BOSS_LICH: индекс следующего HP-порога (75/50/25%) для щита+спавна,
    // и миньоны текущей волны (щит снимается, когда все мертвы)
    int nextSummonThreshold = 0;
    std::vector<entt::entity> summonedMinions;
};

struct Equipment {
    int helmetTier = 0, chestTier = 0, leggingsTier = 0, weaponTier = 0;
    enum WeaponType { NONE, SWORD, BOW } weaponType = NONE;
};

struct MetaStats { int strength, endurance, health, points; };
```

---

## 5. ECS — Системы

Порядок вызова в `PlayState::update()`:
1. `MovementSystem`
2. `CollisionSystem`
3. *(clamp Position игрока **и всех `EnemyTag`/боссов** к границам мира —
   см. раздел 7)*
4. `AISystem`
5. `CombatSystem`
6. `StatusEffectSystem`
7. `LootSystem`

Рендер (`PlayState::render`): фон по биому → `RenderSystem` → `HUD` →
`TutorialHint` → (если пауза) `PauseScreen`.

### MovementSystem
Интегрирует `Position` из `Velocity * Speed * dt`. При наличии
`StatusEffect::SLOW` — `speed *= 0.5f`.

### CollisionSystem
После движения разделяет перекрывающиеся сущности с `Position + Renderable + Health`.
Для каждой пары проверяет AABB-перекрытие и смещает обоих на половину глубины
проникновения по оси наименьшего перекрытия. `PhaseThrough` — пропускается.

### CombatSystem
- **Melee (игрок):** атакует **всех** врагов в радиусе `range` от точки
  `Position + Facing * offset` за один удар (не только ближайшего):
  ```cpp
  for (auto candidate : registry.view<EnemyTag, Position, Health>()) {
      // проверить дистанцию, применить урон
  }
  ```
- **Melee (враги):** атакуют только одного ближайшего игрока.
- **Ranged:** создаёт снаряд-entity (`EntityFactory::createProjectile`).
  Снаряды уничтожаются при выходе за `[0, 1280] x [0, 720]` или при попадании
  (`PROJECTILE_HIT_RADIUS = 20`) во врага/игрока в зависимости от `ownerBiome`.
- `BlockAbility`: снижает входящий урон при `isBlocking = true`.
- Статус-эффект атакующего (`StatusEffect`, если есть) переносится на цель
  при попадании melee.
- При убийстве обновляет `Potion.killCounter` атакующего.

**Формула урона** (`CombatSystem::applyDamage`):
```text
if target.Invulnerable -> урон полностью игнорируется (return)

reduction = target.Armor.damageReductionPercent           // от Endurance
if target.BlockAbility.isBlocking:
    reduction += BlockAbility.reductionPercent
reduction = min(reduction, MAX_DAMAGE_REDUCTION = 0.9)

mitigated = (rawDamage - target.Armor.value) * (1 - reduction)
mitigated = max(mitigated, 1)   // минимум 1 урона, если цель не Invulnerable

health.current -= mitigated
-> HitFlash на цель
-> если health.current == 0 и у attacker есть Potion:
       killCounter++; при достижении killsPerCharge -> +1 заряд (до maxCharges)
```

### AISystem
- `CHASE` — движется к игроку.
- `RANGED_KEEP_DISTANCE` — держит дистанцию `range * 0.8`, стреляет.
- `SWARM` — движется с боковым смещением.
- Боссы — см. раздел 8.

Безопасность EnTT: новые сущности (миньоны Лича) собираются в буфер
`PendingSpawn` и создаются **после** основного `for (auto entity : view)`,
чтобы не инвалидировать итераторы пулов `EnemyTag/AIBehavior/Position/Velocity`.

### StatusEffectSystem
- `POISON`: накапливает урон через `damageAccumulator`, применяет целыми
  единицами; пропускается, если у сущности есть `Invulnerable`.
- `SLOW`: читается в `MovementSystem`.
- Удаляет компонент по истечении `duration`.

### LootSystem
При `Health::current == 0`:
- `ReviveOnce` (не использован) → воскрешает с `max/2` HP, не уничтожается.
- Иначе — кандидат на уничтожение.
- **Гарантированный дроп**: если уничтожение текущих кандидатов опустошает
  волну текущего биома (`waveEnemies`), последняя обработанная сущность
  гарантированно роняет фрагмент ключа и экипировку (минуя ролл шанса).
- `KeyFragmentDrop.chance` → `KeyFragmentHolder.count++`, `RunSummary.keyFragments++`.
- `EquipmentDrop.chance` → выбирает случайный **ещё не выпадавший на этом
  тире** слот (0=шлем, 1=кираса, 2=поножи, 3=оружие; тир = `EnemyTag.biome`),
  применяет к `Equipment` игрока (`droppedSlots[tier-1][slot]`). Оружие по
  умолчанию — `SWORD`, если ещё не задано.
- Возвращает `LootResult { kills, fragmentsCollected, equipmentDropped, droppedSlot, droppedTier }`.
- После дропа экипировки `PlayState` вызывает
  `EntityFactory::applyEquipmentStats` (пересчёт армора/HP/урона/скорости/
  боевых компонентов) и `HUD::showEquipmentDrop`.

### RenderSystem
Рисует `sf::RectangleShape` для всех `Position + Renderable`. При `HitFlash` —
lerp цвета к красному. Дополнительно:
- **Оверлей щита** для `Invulnerable` — полупрозрачный фиолетовый
  прямоугольник `Renderable.size + (16,16)` с контуром, рисуется над основным
  спрайтом (используется фазой щита BOSS_LICH).
- **HP-бар и неймплейт** для всех `EnemyTag + Health` (включая боссов): узкая
  полоска HP над сущностью + текст имени (`Name.id` →
  `localization["enemy.<id>"]`), если есть шрифт.

---

## 6. Мир и биомы

Линейная прогрессия: Лес → Пустыня → Зима → Мёртвые земли → Комната босса.

Переход открывается при `keyFragmentsCollected >= 5`. Тема комнаты босса
выбирается рандомно при `World::generate()`.

При переходе: `clearCurrentBiomeEnemies()` → `setCurrentBiome()` →
`spawnBiomeEnemies()` (или `enterBossRoom()`, если текущий биом — Deadlands).

**Спавн:** враги появляются не ближе **200 единиц** от центра экрана
(начальная позиция игрока), до 20 попыток.

**Индикатор перехода:** когда `world.getCurrentBiome().isUnlocked()`, HUD
показывает текст-подсказку (`hud.nextbiome`) в верхней части экрана; клавиша
**F** вызывает `advanceToNextBiome()` (переход не автоматический).

---

## 7. Границы мира и коллизии

Константы `WORLD_WIDTH/HEIGHT = 1280×720` совпадают с
`Game::LOGICAL_WIDTH/HEIGHT` и используются в `AISystem` (клампинг позиций
спавна миньонов) и `CombatSystem` (уничтожение снарядов за границей).

В `PlayState::update`, после `MovementSystem + CollisionSystem`:
1. Игрок клампится к `[size/2, 1280 - size/2] × [size/2, 720 - size/2]`.
2. Все сущности с `EnemyTag + Position + Renderable` (включая боссов) тоже
   клампятся к тем же границам — устраняет уход врагов за пределы экрана из-за
   `CollisionSystem` у края арены или из-за спавна миньонов со случайным
   офсетом.

`CollisionSystem` сама по себе не знает о границах мира — раздвигает только
взаимно пересекающиеся сущности; ограничение экрана — отдельный шаг в
`PlayState::update`.

---

## 8. Конфиги (`assets/config/`)

Все значения баланса — в JSON, без перекомпиляции. Загружаются через
`ConfigLoader::get().loadAll("assets/config")` (синглтон).

### player.json (актуальные значения баланса)
```json
{
  "baseSpeed": 220,
  "baseHealth": 100,
  "meleeRange": 55,
  "meleeCooldown": 0.6,
  "meleeOffset": 20,
  "baseDamage": 8,
  "rangedRange": 300,
  "rangedCooldown": 1.0,
  "rangedProjectileSpeed": 400,
  "potionHealAmount": 50,
  "potionMaxCharges": 3,
  "potionKillsPerCharge": 5
}
```

### enemies.json (ключевые ограничения баланса)
- Ближники (`CHASE`, `SWARM`): `speed` ≤ 130, `range` = 35–40.
- Дальники (`RANGED_KEEP_DISTANCE`): `cooldown` ≥ 2.0.
- Каждый враг несёт `keyFragmentDropChance` и `equipmentDropChance`
  (по умолчанию у `EnemyTemplate` — 0.1), а также опционально `statusEffect`,
  `revivesOnce`, `phaseThrough`.

### equipment.json (тиры 1–4)

| Tier | Шлем (armor/HP) | Кираса (armor/HP) | Поножи (armor/HP/speed) | Меч (dmg/atkSpd/block/range) | Лук (dmg/atkSpd/range) |
|------|-----------------|--------------------|--------------------------|-------------------------------|--------------------------|
| 1 | 1/5 | 2/10 | 1/5/5 | 10/1.5/0.20/45 | 8/1.0/280 |
| 2 | 2/10 | 4/20 | 2/10/10 | 15/1.6/0.25/48 | 12/1.1/300 |
| 3 | 3/15 | 6/30 | 3/15/15 | 22/1.7/0.30/50 | 18/1.2/320 |
| 4 | 4/20 | 8/40 | 4/20/20 | 30/1.8/0.35/52 | 25/1.3/340 |

Загружается через `ConfigLoader::getEquipmentConfig()` в
`EntityFactory::applyEquipmentStats`, который пересчитывает `Armor`,
`Health.max`, `Speed`, `Damage`, `MeleeCombat`/`RangedCombat`/`BlockAbility`
игрока при создании и при каждом дропе экипировки.

### bosses.json
4 записи (id, theme, health/damage/armor/speed, behavior, цвет/размер) — см.
таблицу в разделе 9.

---

## 9. Meta-прогрессия

Сохраняется в `meta_save.json` (plain text). Загружается при старте,
сохраняется **только при нажатии CONTINUE** в `MetaUpgradeState`.

| Событие | Баллы |
|---------|-------|
| 10 убийств | +1 (`POINTS_PER_10_KILLS`) |
| Биом пройден | +2 (`POINTS_PER_BIOME`) |
| Босс побеждён | +5 (`POINTS_FOR_BOSS`) |
| Фрагмент ключа | +0 (`POINTS_PER_FRAGMENT`, легко изменить) |

Очки тратятся 1:1 на уровни `Strength` / `Endurance` / `Health`
(`spendPointOn*`, экран `MetaUpgradeScreen`).

### Эффекты — процентные, не абсолютные

Каждый уровень даёт **% от итогового значения**, применяемый как множитель
**после** учёта экипировки — бонусы остаются значимыми на любом тире
снаряжения:

| Стат | Константа (`MetaProgression.hpp`) | Эффект |
|------|-----------------------------------|--------|
| Strength | `DAMAGE_PERCENT_PER_STRENGTH = 0.02` | `Damage = round(baseWeaponDamage * (1 + strength * 0.02))` — +2% урона за уровень |
| Endurance | `DAMAGE_REDUCTION_PERCENT_PER_ENDURANCE = 0.02` | `Armor.damageReductionPercent = endurance * 0.02` — +2% снижения урона за уровень (суммируется с блоком, капается на 90% в `CombatSystem`) |
| Health | `HEALTH_PERCENT_PER_LEVEL = 0.02` | `Health.max = round((baseHealth + equipmentHealthBonus) * (1 + health * 0.02))` — +2% максимального HP за уровень |

Применяется в `EntityFactory::applyEquipmentStats`, вызываемом при создании
игрока и при каждом дропе экипировки.

---

## 10. Боссы

Комната босса доступна только из Биома 4 (Deadlands). Тема рандомная,
босс определяется темой через `bosses.json`. Все боссы создаются через
`EntityFactory::createBoss`: получают `BossTag`,
`BossAI{phaseTimer=0, phaseIndex=0, baseSpeed=tmpl.speed, rangedMode=false}`,
и боевой компонент — `RangedCombat` для `BOSS_ELEMENTAL`, иначе `MeleeCombat`
(range = max(size.x, size.y), cooldown = 1.0).

| Тема | Босс (`id`) | HP | Урон | Броня | Speed | Поведение |
|------|-------------|----|------|-------|-------|-----------|
| Лес | `elder_druid` | 600 | 18 | 6 | 70 | `BOSS_DRUID` |
| Пустыня | `sand_naga` | 700 | 22 | 8 | 65 | `BOSS_NAGA` |
| Зима | `frost_elemental` | 750 | 25 | 10 | 55 | `BOSS_ELEMENTAL` |
| Мёртвые земли | `ancient_lich` | 850 | 30 | 8 | 60 | `BOSS_LICH` |

### BOSS_DRUID (Elder Druid)
Каждые `DRUID_PHASE_DURATION = 3.0s` циклически меняет фазу (`phaseIndex % 3`),
меняя множитель скорости и `MeleeCombat.range`:
- Bear (0): ×0.6 скорости, range 90 (медленный, широкий удар);
- Hawk (1): ×1.8 скорости, range 35 (быстрый, короткий удар);
- Fox (2): ×1.1 скорости, range 55 (баланс).
Движение — постоянное преследование игрока (`CHASE`-логика).

### BOSS_NAGA (Sand Naga)
Каждые `NAGA_PHASE_DURATION = 4.0s` переключает `rangedMode`:
- ranged-фаза: снимает `MeleeCombat`, ставит
  `RangedCombat{range=260, projSpeed=320, cooldown=1.2}`, движение — держит
  дистанцию (`applyKeepDistance`, целевая дистанция = `range*0.8`);
- melee-фаза: снимает `RangedCombat`, ставит
  `MeleeCombat{range=max(size), cooldown=1.0}`, движение — преследование.

### BOSS_ELEMENTAL (Frost Elemental)
Только `RangedCombat{range=260, projSpeed=320, cooldown=1.2}` с самого начала.
Каждые `ELEMENTAL_SLOW_INTERVAL = 4.0s` накладывает на игрока
`StatusEffect::SLOW` длительностью `ELEMENTAL_SLOW_DURATION = 2.0s`. Движение —
всегда держит дистанцию (`applyKeepDistance`, целевая дистанция = `range*0.8`).

### BOSS_LICH (Ancient Lich)

**Движение.** Лич преследует игрока как обычный `CHASE`-моб, используя
`MeleeCombat{range=72, cooldown=1.0}`, выставленный в `EntityFactory::createBoss`.

**Фазы щита по HP-порогам** (`LICH_SUMMON_THRESHOLDS = {0.75, 0.50, 0.25}`):

1. Каждый кадр, пока Лич **не** под щитом, проверяется
   `health.current / health.max <= LICH_SUMMON_THRESHOLDS[nextSummonThreshold]`.
2. При срабатывании:
   - `nextSummonThreshold++` (75% → 50% → 25%, максимум 3 срабатывания за бой);
   - Личу добавляется `Invulnerable` — он становится **неуязвимым** (любой
     `applyDamage` по нему игнорируется, тик яда от `StatusEffectSystem` тоже
     пропускается);
   - спавнится волна из **2–3 случайных врагов биома Deadlands**
     (`ConfigLoader::getEnemyConfig().getEnemiesForBiome(DEADLANDS)`), позиции —
     случайный офсет ±80 от позиции босса, кламп к границам мира с отступом 40px;
   - все созданные сущности записываются в `BossAI.summonedMinions`.
3. Пока Лич под щитом, каждый кадр проверяется, остался ли хоть один живой
   (`registry.valid(...)`) миньон из `summonedMinions`. Если **все миньоны
   мёртвы** → `Invulnerable` снимается, `summonedMinions` очищается, Лич снова
   получает урон.
4. Если игрок наносит урон, опускающий HP сразу за несколько порогов,
   фазы срабатывают **последовательно**: щит поднимается → миньоны убиты →
   щит снова поднимается на следующем пороге → и так далее, пока
   `nextSummonThreshold` не достигнет 3.

**Визуализация щита** (`RenderSystem.cpp`): любая сущность с `Invulnerable`
получает оверлей — полупрозрачный ярко-фиолетовый прямоугольник
(`fillColor = RGBA(180,70,255,90)`, `outlineColor = RGBA(220,140,255,220)`,
`outlineThickness = 2`), размером `Renderable.size + (16,16)`, отрисовываемый
сразу после основного спрайта.

Победа: все сущности с `BossTag + Health::current == 0`.

---

## 11. Технические решения

- **Коллизии:** AABB между юнитами (`CollisionSystem`). Коллизии со стенами
  биомов — будущая итерация.
- **Сохранение:** plain text JSON, без шифрования.
- **Музыка:** `sf::Music` (стриминг), уникальный трек на биом + меню + босс.
- **Локализация:** RU / EN через кнопки в главном меню, `Localization::load()`
  перезагружает строки на лету. Неймплейты требуют ключей `enemy.<id>` в
  `ru.json`/`en.json` для каждого `id` из `enemies.json`/`bosses.json`.
- **sf::Text:** не создаётся в циклах рендера — хранится в полях классов UI
  и обновляется только при изменении значений (`textInitialized`).
- **AudioManager:** `std::list<sf::Sound>` вместо `std::vector` для стабильных
  указателей.
- **Безопасность итерации EnTT:** новые сущности (миньоны Лича) создаются
  через `PendingSpawn`-буфер после основного цикла `view`, чтобы не
  инвалидировать итераторы.
- **PauseScreen:** отдельный модальный UI, рисуется поверх замороженного мира
  (Escape).

---

## 12. Актуальные задачи (v8)

### Графика
- [ ] **Спрайты** — `assets/textures/` пуст, всё ещё рендерится
  прямоугольниками (`Renderable`). Требуется: загрузка текстур, замена
  `sf::RectangleShape` на `sf::Sprite` в `RenderSystem`, анимации (как минимум
  idle/walk/attack для игрока и врагов).

### Баланс / Прогрессия
- [ ] **`POINTS_PER_FRAGMENT = 0`** — решить, давать ли meta-очки за фрагменты
  ключа отдельно от прогресса по биому (сейчас фрагменты влияют только на
  открытие перехода).
- [ ] **Уникальные бонусы экипировки** (лайфстил, % урона по типу цели и т.п.,
  упомянутые в `gdd.md`) — `ArmorPieceStats`/`WeaponStats` пока содержат
  только базовые поля (armor/HP/speed/damage/atkSpeed/block/range).

### Мир
- [ ] **Процедурная генерация / чанки биомов** из `gdd.md` — сейчас биомы это
  фиксированные плоские арены 1280×720 со случайным спавном врагов; нет стен
  и препятствий, коллизии со средой не реализованы (только между юнитами).

### UI / Полировка
- [ ] **Пауза** реализована (Resume/Main Menu) — рассмотреть добавление пункта
  "Настройки" (звук, язык) прямо из `PauseScreen`, без выхода в главное меню.
- [ ] **Звук** — `assets/sounds/music/` и `sfx/` пока содержат только
  `.gitkeep`; треки по биомам/боссу и SFX (удар, смерть, дроп, level-up меты)
  не подключены.
