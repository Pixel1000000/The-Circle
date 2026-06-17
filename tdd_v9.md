# Technical Design Document
## The Circle — C++ / SFML / EnTT (v8)

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

Зависимости управляются через **Conan 2**. `conan install . --build=missing` перед первой сборкой.

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
│   └── textures/            # пока пусто — рендер прямоугольниками
└── src/
    ├── main.cpp
    ├── Game.hpp/.cpp
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
    │   ├── EquipmentConfig.hpp
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

| Состояние | Переход |
|-----------|---------|
| `MainMenuState` | → `PlayState` |
| `PlayState` | → `DeathState` (смерть или победа) |
| `DeathState` | → `MetaUpgradeState` |
| `MetaUpgradeState` | → `MainMenuState` |

Смена состояний отложена (`pendingAction`). Окно масштабируемое — `sf::View` 1280×720, letterbox. `applyLetterboxView()` в конструкторе и при `Resized`.

### Пауза
Escape переключает `paused`. При паузе: ECS не обновляется, рисуется `PauseScreen` (Resume / Main Menu). Main Menu — без сохранения текущего рана.

---

## 4. ECS — Компоненты

```cpp
struct Position       { float x, y; };
struct Velocity       { float dx, dy; };
struct Facing         { float dx = 0, dy = 1; };
struct Health         { int current, max; };
struct Armor          { int value; float damageReductionPercent = 0.f; };
struct Damage         { int value; };
struct Speed          { float value; };

struct PlayerTag      {};
struct BossTag        {};
struct EnemyTag       { int biome; };       // 1–4
struct Name           { std::string id; };  // ключ локализации "enemy.<id>"
struct ProjectileTag  { int ownerBiome; };  // 0 = снаряд игрока
struct PhaseThrough   {};
struct Invulnerable   {};                   // полный иммунитет к урону и DoT

struct Renderable     { sf::Color color; sf::Vector2f size; };
struct HitFlash       { static constexpr float DURATION = 0.15f; float timer = DURATION; };

struct MeleeCombat    { float range; float cooldown; float timer; float offset; };
struct RangedCombat   { float range; float projectileSpeed; float cooldown; float timer; };
struct BlockAbility   { float reductionPercent; bool isBlocking; };

struct Potion         { int healAmount; int charges; int maxCharges;
                        int killsPerCharge; int killCounter; };

struct StatusEffect   { enum Type { POISON, SLOW } type;
                        float dps; float duration; float timer;
                        float damageAccumulator = 0.f; };

struct KeyFragmentDrop   { float chance; };
struct EquipmentDrop     { float chance = 0.1f; };
struct KeyFragmentHolder { int count; };
struct ReviveOnce        { bool used; };

struct AIBehavior { enum Type { CHASE, RANGED_KEEP_DISTANCE, SWARM,
                                BOSS_DRUID, BOSS_NAGA, BOSS_ELEMENTAL, BOSS_LICH } type; };

struct BossAI {
    float phaseTimer = 0.f;
    int phaseIndex = 0;
    float baseSpeed = 0.f;
    bool rangedMode = false;
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

Порядок в `PlayState::update()`:
1. `MovementSystem`
2. `CollisionSystem`
3. Clamp позиций (игрок + все `EnemyTag`) к `[size/2, 1280-size/2] × [size/2, 720-size/2]`
4. `AISystem`
5. `CombatSystem`
6. `StatusEffectSystem`
7. `LootSystem`

### MovementSystem
`Position += Velocity * Speed * dt`. При `StatusEffect::SLOW` — `speed *= 0.5f`.

### CollisionSystem
AABB push-apart для всех пар `Position + Renderable + Health`. Смещает обоих на половину глубины проникновения по оси наименьшего перекрытия. `PhaseThrough` — пропускается.

### CombatSystem
**Melee (игрок):** атакует **всех** врагов в `range` от точки `Position + Facing * offset` за один удар.
**Melee (враги):** только ближайший игрок.
**Ranged:** создаёт снаряд через `EntityFactory::createProjectile`. Снаряды уничтожаются за границей `[0,1280]×[0,720]` или при попадании (`PROJECTILE_HIT_RADIUS = 20`).
**Статус-эффект** атакующего переносится на цель при melee-попадании.

**Формула урона:**
```
if Invulnerable → игнорировать

reduction = Armor.damageReductionPercent
if BlockAbility.isBlocking: reduction += reductionPercent
reduction = min(reduction, 0.9)

mitigated = max((rawDamage - Armor.value) * (1 - reduction), 1)
health.current -= mitigated → HitFlash
if health == 0 и attacker.Potion: killCounter++ → при killsPerCharge: +1 заряд
```

### AISystem
- `CHASE` — преследование
- `RANGED_KEEP_DISTANCE` — дистанция `range * 0.8`, стрельба
- `SWARM` — преследование с боковым смещением
- Боссы — см. раздел 9

Миньоны Лича создаются через `PendingSpawn`-буфер после основного цикла view.

### StatusEffectSystem
- `POISON`: `damageAccumulator += dps * dt`, применять целыми единицами. Пропускать при `Invulnerable`.
- `SLOW`: читается в MovementSystem.
- Удалять по истечении `duration`.

### LootSystem
При `Health::current == 0`:
- `ReviveOnce` (не использован) → воскресить с `max/2` HP.
- Иначе — уничтожить. При опустошении волны биома — гарантированный дроп.
- `KeyFragmentDrop.chance` → `KeyFragmentHolder.count++`
- `EquipmentDrop.chance` → случайный незадропавший слот тира `EnemyTag.biome`, применить через `EntityFactory::applyEquipmentStats`
- Возвращает `LootResult { kills, fragmentsCollected, equipmentDropped, droppedSlot, droppedTier }`

### RenderSystem
`sf::RectangleShape` для `Position + Renderable`. `HitFlash` — lerp к красному. Дополнительно:
- **Оверлей щита** при `Invulnerable`: полупрозрачный фиолетовый прямоугольник `size + (16,16)`, `fill = RGBA(180,70,255,90)`, `outline = RGBA(220,140,255,220)`, толщина 2.
- **HP-бар и неймплейт** для всех `EnemyTag + Health + Name`: полоска HP + текст `localization["enemy.<id>"]` над сущностью.

---

## 6. Мир и биомы

Линейная прогрессия: Лес → Пустыня → Зима → Мёртвые земли → Комната босса.

Переход при `keyFragmentsCollected >= 5`. Тема комнаты босса — рандом при `World::generate()`.

**Переход:** клавиша **F** при `biome.isUnlocked()` → `advanceToNextBiome()`. HUD показывает подсказку (`hud.nextbiome`) вверху экрана.

**Спавн:** не ближе 200 единиц от центра, до 20 попыток.

---

## 7. Конфиги

### player.json
```json
{
  "baseSpeed": 220, "baseHealth": 100,
  "meleeRange": 55, "meleeCooldown": 0.6, "meleeOffset": 20, "baseDamage": 8,
  "rangedRange": 300, "rangedCooldown": 1.0, "rangedProjectileSpeed": 400,
  "potionHealAmount": 50, "potionMaxCharges": 3, "potionKillsPerCharge": 5
}
```

### enemies.json (ограничения баланса)
- Ближники: `speed` ≤ 130, `range` 35–40
- Дальники: `cooldown` ≥ 2.0

### equipment.json (тиры 1–4)

| Tier | Шлем armor/HP | Кираса armor/HP | Поножи armor/HP/spd | Меч dmg/atkSpd/block/range | Лук dmg/atkSpd/range |
|------|--------------|-----------------|---------------------|---------------------------|----------------------|
| 1 | 1/5 | 2/10 | 1/5/5 | 10/1.5/0.20/45 | 8/1.0/280 |
| 2 | 2/10 | 4/20 | 2/10/10 | 15/1.6/0.25/48 | 12/1.1/300 |
| 3 | 3/15 | 6/30 | 3/15/15 | 22/1.7/0.30/50 | 18/1.2/320 |
| 4 | 4/20 | 8/40 | 4/20/20 | 30/1.8/0.35/52 | 25/1.3/340 |

---

## 8. Meta-прогрессия

Сохраняется в `meta_save.json`. Сохранение **только при CONTINUE** в `MetaUpgradeState`.

| Событие | Баллы |
|---------|-------|
| 10 убийств | +1 |
| Биом пройден | +2 |
| Босс побеждён | +5 |
| Фрагмент ключа | +0 *(`POINTS_PER_FRAGMENT`)* |

### Эффекты (процентные, после учёта экипировки)

| Стат | Константа | Эффект |
|------|-----------|--------|
| Strength | `DAMAGE_PERCENT_PER_STRENGTH = 0.02` | `Damage = round(baseWeaponDamage * (1 + strength * 0.02))` |
| Endurance | `DAMAGE_REDUCTION_PERCENT_PER_ENDURANCE = 0.02` | `Armor.damageReductionPercent = endurance * 0.02` (кап 90%) |
| Health | `HEALTH_PERCENT_PER_LEVEL = 0.02` | `Health.max = round((base + equipBonus) * (1 + health * 0.02))` |

---

## 9. Боссы

| Тема | id | HP | Урон | Броня | Speed |
|------|----|----|------|-------|-------|
| Лес | `elder_druid` | 600 | 18 | 6 | 70 |
| Пустыня | `sand_naga` | 700 | 22 | 8 | 65 |
| Зима | `frost_elemental` | 750 | 25 | 10 | 55 |
| Мёртвые земли | `ancient_lich` | 850 | 30 | 8 | 60 |

Победа: все `BossTag + Health::current == 0`.

### BOSS_DRUID
Каждые 3с циклически меняет фазу (`phaseIndex % 3`):
- Bear: ×0.6 speed, range 90
- Hawk: ×1.8 speed, range 35
- Fox: ×1.1 speed, range 55

### BOSS_NAGA
Каждые 4с переключает `rangedMode`:
- Ranged: `RangedCombat{260, 320, 1.2}`, держит дистанцию
- Melee: `MeleeCombat{max(size), 1.0}`, преследование

### BOSS_ELEMENTAL
Только `RangedCombat{260, 320, 1.2}`, держит дистанцию. Каждые 4с накладывает `StatusEffect::SLOW` на игрока (2с).

### BOSS_LICH
Преследует и атакует melee. При HP-порогах 75% / 50% / 25%:
1. Добавляется `Invulnerable` (щит)
2. Спавнятся 2–3 случайных врага Deadlands (±80 от позиции босса)
3. Щит снимается когда **все миньоны мертвы**
4. Щит визуализируется оверлеем в `RenderSystem`

---

## 10. Технические решения

- **Коллизии:** AABB между юнитами. Коллизии со стенами — будущая итерация.
- **Сохранение:** plain text JSON.
- **Музыка:** `sf::Music` стриминг, трек на биом + меню + босс.
- **Локализация:** RU/EN кнопками в меню, `Localization::load()` на лету. Ключи `enemy.<id>` для неймплейтов.
- **sf::Text:** хранится в полях классов UI, не создаётся в циклах рендера.
- **AudioManager:** `std::list<sf::Sound>` для стабильных указателей.
- **EnTT итерация:** миньоны Лича создаются через `PendingSpawn`-буфер.

---

## 11. Актуальные задачи (v9)

### Элементальная система (приоритет)

- [x] **Новые компоненты** — добавить в `Components.hpp`:
  ```cpp
  enum class Element { NONE, NATURE, FIRE, ICE, DECAY };

  struct ElementalEffect {
      Element element = Element::NONE;
      float percent = 0.f;      // % эффекта (5–50% в зависимости от тира)
  };

  // Активный элементальный статус на цели
  struct NatureStack  { float dpsMultiplier = 1.f; float duration; float timer; int stacks = 0; };
  struct FireBurn     { float dps; float duration; float timer; };       // продлевается, не стакается
  struct IceChill     { float slowDuration; };                           // замедление уже через StatusEffect::SLOW
  struct DecayEffect  { int maxHpReduction; float duration; float timer; }; // временное снижение макс. HP
  ```

- [x] **Элемент в экипировке** — расширить `Equipment`:
  ```cpp
  struct Equipment {
      // ... существующие поля ...
      Element weaponElement = Element::NONE;
      float weaponElementPercent = 0.f;
      Element helmetElement = Element::NONE;   float helmetElementPercent = 0.f;
      Element chestElement = Element::NONE;    float chestElementPercent = 0.f;
      Element leggingsElement = Element::NONE; float leggingsElementPercent = 0.f;
      bool autoUpgradeWeapon   = false;
      bool autoUpgradeHelmet   = false;
      bool autoUpgradeChest    = false;
      bool autoUpgradeLeggings = false;
  };
  ```

- [x] **Генерация элемента при дропе** — в `LootSystem` при дропе предмета:
  - Элемент определяется биомом врага: Лес→NATURE, Пустыня→FIRE, Зима→ICE, Мёртвые земли→DECAY
  - % эффекта — рандом в диапазоне тира: T1: 5–10%, T2: 10–20%, T3: 20–35%, T4: 35–50%

- [x] **Экран выбора предмета** — новый UI `ItemChoiceScreen`:
  - Показывается при дропе предмета если `autoUpgrade[slot] == false`
  - Игра замораживается пока открыт экран (аналогично паузе — ECS не обновляется)
  - Два варианта: "Взять новый" / "Апгрейдить текущий"
  - Галочка "Больше не спрашивать" — устанавливает `autoUpgrade[slot] = true`
  - Если `autoUpgrade == true` — апгрейд без диалога

- [x] **Логика апгрейда** — в `EntityFactory::applyEquipmentStats` или отдельный `ItemUpgrader`:
  - Тир предмета повышается до нового
  - % эффекта: `newPercent = oldPercent + (droppedPercent * 0.5f)` (апгрейд даёт половину нового % сверху)

- [x] **Применение эффектов в CombatSystem** — при melee/projectile попадании проверять `Equipment.weaponElement` игрока и накладывать соответствующий компонент на цель:
  - NATURE → `NatureStack`: стакать `dpsMultiplier *= (1 + percent)`, обновить `duration`
  - FIRE → `FireBurn`: если уже есть — продлить `timer`, иначе создать
  - ICE → нанести доп. урон `baseDamage * percent`, наложить `StatusEffect::SLOW` на `slowDuration`
  - DECAY → `DecayEffect`: снизить `Health.max` на `max * percent`, кламп `current` до нового `max`

- [x] **Обработка элементов в StatusEffectSystem** — добавить тики для `NatureStack`, `FireBurn`, `DecayEffect`. При истечении `DecayEffect` — восстановить `Health.max`.

- [x] **Тематические бонусы брони** — в `EntityFactory::applyEquipmentStats`:

  | Слот/Элемент | Эффект |
  |---|---|
  | Шлем/Кираса NATURE | `resistFire`: снижает урон от `FireBurn` на `percent` |
  | Шлем/Кираса FIRE | `resistNature`: снижает DoT от `NatureStack` на `percent` |
  | Шлем/Кираса ICE | `resistSlow`: уменьшает длительность `StatusEffect::SLOW` на `percent` |
  | Шлем/Кираса DECAY | `resistDecay`: снижает `maxHpReduction` от `DecayEffect` на `percent` |
  | Поножи NATURE | Лайфстил: при движении (`Velocity != 0`) восстанавливать `speed * percent * dt` HP |
  | Поножи FIRE | Бонус скорости: `Speed += baseSpeed * percent` |
  | Поножи ICE | Иммунитет к замедлению: игнорировать `StatusEffect::SLOW` |
  | Поножи DECAY | Бонус к зелью: `Potion.healAmount *= (1 + percent)` |

  Резисты хранить в новом компоненте:
  ```cpp
  struct ElementalResist {
      float fireResist = 0.f;
      float natureResist = 0.f;
      float slowResist = 0.f;    // 1.0 = иммунитет
      float decayResist = 0.f;
  };
  ```

### Враги — элементы и абилки

- [ ] **Элементальный урон врагов** — добавить в `EnemyTemplate` поля:
  ```cpp
  Element element = Element::NONE;
  float elementPercent = 0.f;  // задаётся в enemies.json
  ```
  В `CombatSystem` при попадании врага по игроку — накладывать элементальный эффект аналогично оружию игрока.
  *(не реализовано: в `EnemyTemplate` (`EnemyConfig.hpp`) полей `element`/`elementPercent` нет, враги не наносят элементальный урон)*

- [ ] **Новые компоненты абилок** — добавить в `Components.hpp`:
  ```cpp
  struct DashAbility    { float cooldown; float timer; float duration; float durationTimer; bool dashing; };
  struct BurrowAbility  { float hpThreshold; float duration; float timer; bool burrowed; };
  struct TrapSpawner    { int count; float radius; float stunDuration; };         // Энт
  struct SwarmScatter   { float duration; float timer; bool scattered; };          // Рой ос
  struct ChargeAbility  { float cooldown; float timer; float speed; bool charging;
                          sf::Vector2f chargeDir; float stunOnFail; };             // Наездник
  struct FreezeOnDeath  { float freezeDuration; float stunRadius; float stunDuration; }; // Гоблин
  struct TeleportAbility{ float cooldown; float timer; };                          // Ведьма/Дух
  struct RageAbility    { float hpThreshold; float speedMult; float damageMult; bool enraged; }; // Йети
  struct AbsorbChance   { float chance; float accumulatedDamage; float releaseTimer; float releaseInterval; }; // Призрак
  struct BoneDetach     { float hpThreshold; float interval; float timer; int detached; int maxDetach; }; // Голем
  struct SkeletonReviveBonus { float invulDuration; float invulTimer; float damageMult; bool bonusActive; };
  struct QuicksandSpawner { float cooldown; float timer; sf::Vector2f size; float duration; };  // Песчаный дух
  struct MummyDeathBomb { float delay; float timer; bool triggered; float radius; float stunDuration;
                          int blinkCount; int currentBlink; float blinkInterval; float blinkTimer; };
  ```
  *(не реализовано: ни один из этих компонентов отсутствует в `Components.hpp`)*

- [ ] **Новые теги/зоны** — добавить:
  ```cpp
  struct TrapTag        {};   // ловушки Энта
  struct QuicksandTag   {};   // зыбучие пески (глобальный лимит 3)
  struct IceZoneTag     {};   // ледяные зоны Ведьмы
  ```
  *(не реализовано)*

- [ ] **AbilitySystem** — новая система `src/ecs/systems/AbilitySystem.hpp/.cpp`, вызывается в `PlayState::update()` между `AISystem` и `CombatSystem`. Обрабатывает все абилки по компонентам:
  - `DashAbility` → Волк: рывок к игроку + `Invulnerable` на время рывка
  - `BurrowAbility` → Скорпион: закапывание при HP < threshold
  - `TrapSpawner` → Энт: спавн ловушек при смерти (через `LootSystem` или `DeathCallback`)
  - `SwarmScatter` → Рой ос: scatter при получении урона
  - `ChargeAbility` → Наездник: таран, обработка столкновений
  - `FreezeOnDeath` → Ледяной гоблин: заморозка + взрыв при смерти
  - `TeleportAbility` → Снежная ведьма + Ледяной дух
  - `RageAbility` → Йети: ярость при HP < threshold
  - `AbsorbChance` → Призрак: поглощение урона + выплеск
  - `BoneDetach` → Костяной голем: отделение скелетов
  - `QuicksandSpawner` → Песчаный дух: зыбучие пески (лимит 3 на карте)
  - `MummyDeathBomb` → Мумия: telegraphed взрыв с миганием
  - `SkeletonReviveBonus` → Скелет-воин: бонус после воскрешения
  *(не реализовано: файла `AbilitySystem.hpp/.cpp` не существует, между `AISystem` и `CombatSystem` в `PlayState.cpp` вызывается только `BlizzardSystem`)*

- [ ] **Зоны эффектов** — новая система `ZoneSystem`: каждый кадр проверять AABB игрока и врагов против всех зон (`TrapTag`, `QuicksandTag`, `IceZoneTag`). Применять соответствующие эффекты. Уничтожать зоны по истечении `duration`.
  *(не реализовано: `ZoneSystem` отсутствует)*

- [ ] **enemies.json** — добавить для каждого врага поля `element`, `elementPercent` и параметры абилок (кулдауны, пороги HP, радиусы, длительности).
  *(не реализовано: текущий `assets/config/enemies.json` не содержит этих полей)*

- [ ] **Гоблин-лучник: призыв** — при HP < 30% добавить в `spawnBiomeEnemies` логику экстренного спавна 1 волка рядом с гоблином через `PendingSpawn`-буфер.
  *(не реализовано: такого врага/логики нет)*

### Препятствия
- [x] **Статичные препятствия внутри биомов** — добавить `struct Obstacle {}` тег + `Position + Renderable` без `Velocity`. В `CollisionSystem` обрабатывать столкновения юнитов с препятствиями (только push юнита, препятствие неподвижно). Генерировать процедурно в `spawnBiomeEnemies()`: 4–8 прямоугольников случайного размера, не перекрывающих зону спавна игрока.
  *(реализовано: `Obstacle` в `Components.hpp`, обработка в `CollisionSystem.cpp`, генерация в `PlayState::spawnObstacles`)*

### Баланс
- [ ] **`POINTS_PER_FRAGMENT`** — установить в 1 (сейчас 0).
  *(не выполнено: в `MetaProgression.hpp` константа всё ещё равна 0)*
- [x] **Уникальные бонусы экипировки** — реализованы через элементальную систему выше.

- [x] **BOSS_ELEMENTAL: пурга** — переработать механику замедления. Вместо периодического `StatusEffect::SLOW` на игрока:
  - Новый тег: `struct BlizzardZoneTag {}` — чтобы `LootSystem`, `CombatSystem` и `AISystem` не трогали зоны пурги
  - Новый компонент: `struct BlizzardZone { sf::Vector2f size; bool drifting; sf::Vector2f driftDir; float dirTimer; };`
  - Элементаль каждые `ELEMENTAL_BLIZZARD_INTERVAL = 8с` уничтожает старые зоны (`BlizzardZoneTag`) и создаёт новые entity с `BlizzardZoneTag + Position + BlizzardZone + Renderable` (полупрозрачный голубой прямоугольник):
    - 1 большая (~150×150, `drifting=true`): медленно дрейфует, меняет направление каждые 3–5с
    - 3–6 малых (~60×60, `drifting=false`): статичные, случайные позиции
  - Новая система `BlizzardSystem`: каждый кадр двигать дрейфующие зоны, проверять AABB игрока против всех `BlizzardZone`. Если пересечение — добавить/обновить `StatusEffect::SLOW` с `duration = 0.2f` (эффект держится пока игрок внутри, исчезает сразу при выходе).
  *(реализовано: `Components.hpp`, `BlizzardSystem.cpp`, `ELEMENTAL_BLIZZARD_INTERVAL` в `AISystem.cpp`)*

- [x] **Волновая система с банком** — переработать `PlayState::spawnBiomeEnemies()`:
  - Добавить в `EnemyTemplate` поле `int cost = 3`, в `enemies.json` — поле `cost` для каждого врага
  - Банк волны: `waveBank = BASE_WAVE_BANK + metaTotalPoints * BANK_PER_META_POINT`
    - `BASE_WAVE_BANK = 20`, `BANK_PER_META_POINT = 2`
    - `metaTotalPoints = strength + endurance + health` — сумма **уровней**, не очков. Пример: strength=5, endurance=3, health=2 → metaTotalPoints=10 → bank=40
  - Алгоритм:
    ```cpp
    int bank = computeWaveBank(metaStats);
    auto pool = config.getEnemiesForBiome(biome);
    shuffle(pool);
    while (bank > 0) {
        auto affordable = filter(pool, [bank](e){ return e.cost <= bank; });
        if (affordable.empty()) {
            spawnEnemy(minCostEnemy(pool)); // самый дешёвый, завершить
            break;
        }
        auto& chosen = randomFrom(affordable);
        spawnEnemy(chosen);
        bank -= chosen.cost;
    }
    ```
  *(реализовано в `PlayState.cpp`: `BASE_WAVE_BANK`, `BANK_PER_META_POINT`, shuffle + affordable-отбор)*

- [x] **Настройки в PauseScreen** — добавить кнопку "Настройки" в `PauseScreen`. Вложенный экран:
  - Слайдер громкости музыки (`AudioManager::setMusicVolume(float)`)
  - Слайдер громкости SFX (`AudioManager::setSfxVolume(float)`)
  - Кнопки RU / EN (`Localization::load()` + обновить все активные UI)
  - Кнопка "Назад" → возврат к паузе
  *(реализовано: `src/ui/SettingsScreen.hpp/.cpp`, подключён из `PauseScreen`)*

### Звук
- [ ] **Аудиофайлы** — треки по биомам/боссу и SFX.
  *(не выполнено: `assets/sounds/music` и `assets/sounds/sfx` содержат только `.gitkeep`)*

### Графика
- [ ] **Спрайты** — загрузка текстур, замена `sf::RectangleShape` на `sf::Sprite` в `RenderSystem`, анимации idle/walk/attack для игрока и врагов.
  *(не выполнено: `assets/textures` пуст, `RenderSystem` использует только `sf::RectangleShape`)*

### Баланс
- [ ] **`POINTS_PER_FRAGMENT`** — решить, давать ли очки за фрагменты отдельно от прогресса биома.
- [x] **Уникальные бонусы экипировки** — лайфстил, % урона по типу, бонусы к конкретным статам (упомянуто в GDD). Расширить `ArmorPieceStats`/`WeaponStats`.
  *(реализовано через элементальную систему — см. выше)*

### Мир
- [x] **Препятствия и стены внутри биомов** — сейчас арена полностью открытая. Добавить статичные объекты с AABB-коллизией для тактического разнообразия.
  *(реализовано — см. раздел "Препятствия" выше)*

### UI
- [x] **Настройки из PauseScreen** — громкость звука и музыки, смена языка без выхода в главное меню.
  *(реализовано — `SettingsScreen`)*

### Звук
- [ ] **Аудиофайлы** — `music/` и `sfx/` пусты. Подключить треки по биомам/боссу и SFX (удар, смерть, дроп, level-up меты).
