# Technical Design Document
## The Circle — C++ / SFML / EnTT (v8)

> Обновление TDD v7 по факту реализации. Раздел 11 (задачи v7) выполнен
> полностью — отмечен как done. Добавлены компоненты/системы/механики,
> появившиеся в коде, но не описанные в v7, и новый список задач (раздел 12).

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

Без изменений относительно v7. Conan 2 (`conanfile.txt`), `CMakeDeps` + `CMakeToolchain`.

---

## 2. Файловая структура

Без изменений относительно v7, кроме перечисленных ниже добавлений:

```
TheCircle/
├── ...
├── assets/
│   ├── config/
│   │   ├── player.json
│   │   ├── equipment.json
│   │   ├── enemies.json
│   │   └── bosses.json
│   └── textures/        # ← новое, пока пусто (placeholder-рендер)
└── src/
    ├── config/
    │   ├── ConfigLoader.hpp/.cpp
    │   ├── PlayerConfig.hpp
    │   ├── EquipmentConfig.hpp   # ← новое: ArmorPieceStats/WeaponStats, TIER_COUNT=4
    │   └── EnemyConfig.hpp/.cpp
    └── ui/
        ├── HUD.hpp/.cpp
        ├── MetaUpgradeScreen.hpp/.cpp
        ├── TutorialHint.hpp/.cpp
        └── PauseScreen.hpp/.cpp   # ← новое
```

---

## 3. Game Loop и State Machine

Состояния и переходы — без изменений относительно v7.

**Новое: пауза внутри `PlayState`.** Клавиша **Escape** переключает
`paused`. Пока `paused == true`:
- `update()` не выполняет ни одной системы (`lastDt = 0`, ECS не шагает);
- `render()` дополнительно рисует `PauseScreen` (полупрозрачный оверлей +
  кнопки **Resume** / **Main Menu**);
- клик мыши обрабатывается через `PauseScreen::getButtonAt()` →
  `Resume` снимает паузу, `Main Menu` сразу делает `game.changeState(MainMenuState)`
  (без сохранения прогресса текущего рана).

---

## 4. ECS — Компоненты (`ecs/Components.hpp`)

Базовые компоненты из v7 не изменились (`Position`, `Velocity`, `Facing`,
`Health`, `Damage`, `Speed`, `PlayerTag`, `EnemyTag`, `ProjectileTag`,
`PhaseThrough`, `Renderable`, `HitFlash`, `MeleeCombat`, `RangedCombat`,
`BlockAbility`, `Potion`, `StatusEffect`, `KeyFragmentDrop`,
`KeyFragmentHolder`, `ReviveOnce`, `AIBehavior`, `Equipment`, `MetaStats`).

Изменения и добавления:

```cpp
// Armor — добавлено поле для процентного снижения урона (Endurance-бонус)
struct Armor {
    int value = 0;                       // плоское снижение (было и раньше)
    float damageReductionPercent = 0.0f; // ← новое: from MetaStats.endurance
};

// Name — новое. Неймплейт над HP-баром врага/босса.
// localization key = "enemy.<id>"
struct Name { std::string id; };

// EquipmentDrop — новое, аналог KeyFragmentDrop для дропа экипировки
struct EquipmentDrop { float chance = 0.1f; };

// Invulnerable — новое. Полный иммунитет к урону и тикам яда
// (используется для фазы щита BOSS_LICH).
struct Invulnerable {};

// BossAI — новое. Состояние фаз боссов.
struct BossAI {
    float phaseTimer = 0.0f;
    int phaseIndex = 0;
    float baseSpeed = 0.0f;
    bool rangedMode = false;

    // BOSS_LICH: следующий HP-порог (75/50/25%) для щита+спавна,
    // и список миньонов текущей волны (щит снят, когда все мертвы)
    int nextSummonThreshold = 0;
    std::vector<entt::entity> summonedMinions;
};
```

`BossTag` присутствует как и в v7 (без изменений).

---

## 5. ECS — Системы

Порядок вызова в `PlayState::update()` — как в v7:
1. `MovementSystem`
2. `CollisionSystem`
3. *(между 2 и 3 — clamp Position игрока **и теперь всех `EnemyTag`** к
   границам мира `[0,1280]×[0,720]`, см. раздел 8)*
4. `AISystem`
5. `CombatSystem`
6. `StatusEffectSystem`
7. `LootSystem`

### MovementSystem
Без изменений относительно v7.

### CollisionSystem — **реализовано** (было задачей v7)
AABB push-apart для всех `Position + Renderable + Health`, кроме
`PhaseThrough`. Для каждой пары: если AABB пересекаются, раздвигает по оси
наименьшего перекрытия, по 50% глубины на каждую сторону.

### CombatSystem
- **Melee (игрок) бьёт всех в радиусе — реализовано** (было задачей v7):
  `updateMelee` для `PlayerTag` итерирует по `view<EnemyTag, Position, Health>()`
  и наносит урон каждому в `range` от точки `Position + Facing*offset`.
- **Melee (враги):** только ближайший игрок (как в v7).
- **Ranged:** снаряды через `EntityFactory::createProjectile`.
- **Снаряды за границей экрана — реализовано** (было задачей v7):
  `updateProjectiles` уничтожает снаряды вне `[0,1280]×[0,720]`.
- **`applyDamage` — новая формула** (отличается от v7, где была просто
  `Armor.value`):
  ```text
  if target.Invulnerable -> урон игнорируется полностью

  reduction = target.Armor.damageReductionPercent (Endurance-бонус)
  if BlockAbility.isBlocking: reduction += BlockAbility.reductionPercent
  reduction = min(reduction, 0.9)

  mitigated = max(1, (rawDamage - Armor.value) * (1 - reduction))
  health.current -= mitigated
  -> HitFlash; killCounter++ на Potion атакующего при добивании
  ```

### AISystem
`CHASE`, `RANGED_KEEP_DISTANCE`, `SWARM` — как в v7. Боссы — см. раздел 7
(полностью реализованы, было задачей v7).

Безопасность EnTT: новые сущности (миньоны Лича) собираются в буфер
`PendingSpawn` и создаются **после** основного `for (auto entity : view)`,
чтобы не инвалидировать итераторы.

### StatusEffectSystem
Как в v7, плюс: тик `POISON` пропускается, если у сущности есть
`Invulnerable`.

### LootSystem
- Обработка `ReviveOnce` и дроп фрагментов — как в v7.
- **Дроп экипировки — реализовано** (было задачей v7): при срабатывании
  `EquipmentDrop.chance` выбирается случайный **ещё не выпадавший на этом
  тире** слот (helmet/chest/legs/weapon, тир = `EnemyTag.biome`), применяется
  к `Equipment` игрока, оружие по умолчанию `SWORD`. После — вызывается
  `EntityFactory::applyEquipmentStats` и `HUD::showEquipmentDrop`.
- **Новое:** гарантированный дроп — если уничтожение текущих кандидатов
  опустошает волну биома (`waveEnemies`), последняя обработанная сущность
  гарантированно роняет фрагмент ключа и экипировку (минуя ролл).

### RenderSystem
Как в v7 (прямоугольники + lerp к красному при `HitFlash`), плюс:
- **HP-бар и неймплейт** для всех `EnemyTag + Health` (включая боссов):
  узкая полоска HP над сущностью + текст имени (`Name.id` →
  `localization["enemy.<id>"]`).
- **Оверлей щита** для `Invulnerable` — полупрозрачный фиолетовый
  прямоугольник `Renderable.size + (16,16)` с контуром, рисуется над основным
  спрайтом (используется фазой щита BOSS_LICH).

---

## 6. Мир и биомы

Без изменений относительно v7: линейная прогрессия Лес → Пустыня → Зима →
Мёртвые земли → Комната босса; переход открывается при
`keyFragmentsCollected >= 5`; тема комнаты босса — рандом в `World::generate()`;
спавн врагов не ближе 200 ед. от игрока (до 20 попыток).

**Реализовано (было задачей v7): индикатор перехода.** Когда
`world.getCurrentBiome().isUnlocked()` — HUD показывает текст-подсказку
(`hud.nextbiome`) в верхней части экрана; клавиша **F** вызывает
`advanceToNextBiome()` (переход не автоматический).

---

## 7. Боссы — реализовано (было задачей v7)

Комната босса доступна только из Биома 4 (Deadlands). Тема рандомная (4
варианта), босс определяется темой через `bosses.json`.

| Тема | Босс (`id`) | HP/Урон/Броня/Speed | Поведение |
|------|-------------|----------------------|-----------|
| Лес | `elder_druid` | 600/18/6/70 | `BOSS_DRUID` |
| Пустыня | `sand_naga` | 700/22/8/65 | `BOSS_NAGA` |
| Зима | `frost_elemental` | 750/25/10/55 | `BOSS_ELEMENTAL` |
| Мёртвые земли | `ancient_lich` | 850/30/8/60 | `BOSS_LICH` |

Все создаются через `EntityFactory::createBoss` (`BossTag` + `BossAI`).

- **BOSS_DRUID** — каждые 3с циклически меняет фазу (`phaseIndex % 3`):
  Bear (×0.6 speed, melee range 90) → Hawk (×1.8 speed, range 35) → Fox
  (×1.1 speed, range 55). Движение — постоянный `CHASE`.
- **BOSS_NAGA** — каждые 4с переключает `rangedMode`: ranged-фаза снимает
  `MeleeCombat`, ставит `RangedCombat{260, 320, 1.2}` и держит дистанцию
  (`range*0.8`); melee-фаза — наоборот, `MeleeCombat{max(size), 1.0}` + `CHASE`.
- **BOSS_ELEMENTAL** — только `RangedCombat{260, 320, 1.2}`, держит дистанцию
  `range*0.8`; каждые 4с накладывает на игрока `StatusEffect::SLOW` на 2с.
- **BOSS_LICH** — *новая механика, отсутствовавшая в v7*:
  - Преследует игрока как `CHASE`-моб, `MeleeCombat{range=72, cooldown=1.0}`.
  - На HP-порогах **75% / 50% / 25%** (`LICH_SUMMON_THRESHOLDS`): получает
    `Invulnerable` и спавнит волну из **2–3 случайных врагов биома
    Deadlands** рядом с собой (офсет ±80, клампится к границам мира).
  - Щит снимается, когда **все миньоны волны мёртвы**
    (`BossAI.summonedMinions` проверяется каждый кадр через
    `registry.valid`).
  - При резком падении HP сразу через несколько порогов фазы срабатывают
    последовательно (до 3 раз за бой).

Победа: все сущности с `BossTag + Health::current == 0`.

---

## 8. Границы мира (новое, не было в v7)

Константы `WORLD_WIDTH/HEIGHT = 1280×720` используются в `AISystem` (клампинг
позиций спавна миньонов) и `CombatSystem` (уничтожение снарядов за
границей).

В `PlayState::update`, после `MovementSystem + CollisionSystem`:
1. Игрок клампится к `[size/2, 1280 - size/2] × [size/2, 720 - size/2]`.
2. **Новое:** все `EnemyTag + Position + Renderable` (включая боссов) тоже
   клампятся к тем же границам — устраняет уход врагов за экран из-за
   `CollisionSystem` у края арены или из-за спавна миньонов со случайным
   офсетом.

`CollisionSystem` сама не знает о границах — раздвигает только взаимно
пересекающиеся сущности.

---

## 9. Конфиги (`assets/config/`)

### player.json — баланс из v7 уже применён
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

### enemies.json — ограничения из v7 соблюдены
Ближники (`CHASE`, `SWARM`): `speed` ≤ 130, `range` = 35–40.
Дальники (`RANGED_KEEP_DISTANCE`): `cooldown` ≥ 2.0.
Каждый враг также несёт `keyFragmentDropChance` и (через `EnemyTemplate`,
по умолчанию 0.1) `equipmentDropChance`.

### equipment.json — новое, не описано в v7
Тиры 1–4 для четырёх слотов:

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

---

## 10. Meta-прогрессия — формулы изменились относительно v7

Очки за события — как в v7 (10 убийств → +1, биом пройден → +2, босс → +5,
фрагмент ключа → `POINTS_PER_FRAGMENT = 0`, легко включить).

**Новое:** эффекты потраченных очков теперь **процентные**, а не
фиксированные, применяются как множитель **после** экипировки:

| Стат | Константа | Эффект |
|------|-----------|--------|
| Strength | `DAMAGE_PERCENT_PER_STRENGTH = 0.02` | `Damage = round(baseWeaponDamage * (1 + strength*0.02))` |
| Endurance | `DAMAGE_REDUCTION_PERCENT_PER_ENDURANCE = 0.02` | `Armor.damageReductionPercent = endurance*0.02` (капается на 90% суммарно с блоком) |
| Health | `HEALTH_PERCENT_PER_LEVEL = 0.02` | `Health.max = round((baseHealth + equipBonus) * (1 + health*0.02))` |

Сохраняется в `meta_save.json` (plain text), как в v7.

---

## 11. Технические решения — дополнения к v7

Всё из v7 (AABB-коллизии, plain text save, `sf::Music` per биом, RU/EN
локализация, `sf::Text` не в циклах рендера, `AudioManager` на `std::list`)
остаётся актуальным. Дополнительно:

- **Pending-spawn буфер в AISystem** — новые сущности (миньоны Лича) создаются
  после основного цикла `view`, чтобы не инвалидировать EnTT-итераторы.
- **HUD кеширует `sf::Text`** (`textInitialized`), обновляет только строки.
- **Неймплейты** требуют ключей локализации `enemy.<id>` в `ru.json`/`en.json`
  для каждого `id` из `enemies.json`/`bosses.json`.
- **PauseScreen** — отдельный модальный UI, рисуется поверх замороженного мира.

---

## 12. Актуальные задачи (v8)

### Выполнено (всё из v7, раздел 11)
- [x] CollisionSystem (AABB push-apart, пропуск `PhaseThrough`)
- [x] Melee игрока бьёт всех в радиусе
- [x] Баланс скоростей/дистанций в `player.json`/`enemies.json`
- [x] Уничтожение снарядов за границей экрана
- [x] Дроп экипировки (`EquipmentDrop`, `LootSystem`, `applyEquipmentStats`)
- [x] Индикатор перехода между биомами (F)
- [x] Паттерны всех 4 боссов в `AISystem`
- [x] HUD: строка экипировки и тиры
- [x] HUD: уведомление о дропе экипировки

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
