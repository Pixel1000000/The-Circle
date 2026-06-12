#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace
{
constexpr float kPi = 3.1415926535f;
constexpr unsigned int kWindowWidth = 1280;
constexpr unsigned int kWindowHeight = 720;
const sf::Vector2f kWorldCenter{640.0f, 360.0f};
constexpr float kWorldRadius = 330.0f;
constexpr int kFragmentsForGate = 5;

enum class GameState
{
    Menu,
    Playing,
    Dead,
    Victory
};

enum class WeaponMode
{
    Sword,
    Bow
};

enum class EnemyKind
{
    Melee,
    Ranged,
    Tank,
    Swarm,
    Boss
};

enum class LootKind
{
    Fragment,
    Gear,
    PotionCharge
};

struct Biome
{
    const char* name;
    sf::Color color;
    sf::Vector2f focus;
    int tier;
};

const std::array<Biome, 4> kBiomes{{
    {"Forest", sf::Color(48, 112, 64), {430.0f, 360.0f}, 1},
    {"Desert", sf::Color(176, 140, 72), {640.0f, 150.0f}, 2},
    {"Winter", sf::Color(110, 170, 205), {850.0f, 360.0f}, 3},
    {"Dead Lands", sf::Color(92, 82, 102), {640.0f, 570.0f}, 4},
}};

struct Player
{
    sf::Vector2f position{kBiomes[0].focus};
    float hp = 110.0f;
    float maxHp = 110.0f;
    float speed = 235.0f;
    float attackCooldown = 0.0f;
    int gearTier = 1;
    int fragments = 0;
    int potionCharges = 1;
    int kills = 0;
    WeaponMode weapon = WeaponMode::Sword;
};

struct Enemy
{
    sf::Vector2f position{};
    sf::Vector2f velocity{};
    EnemyKind kind = EnemyKind::Melee;
    float hp = 20.0f;
    float maxHp = 20.0f;
    float speed = 80.0f;
    float damage = 8.0f;
    float radius = 14.0f;
    float attackCooldown = 0.0f;
    float shootCooldown = 0.0f;
    bool boss = false;
    sf::Color color = sf::Color::Red;
};

struct Projectile
{
    sf::Vector2f position{};
    sf::Vector2f velocity{};
    float damage = 0.0f;
    float radius = 5.0f;
    bool fromPlayer = false;
    float lifetime = 3.0f;
    sf::Color color = sf::Color::White;
};

struct Loot
{
    sf::Vector2f position{};
    LootKind kind = LootKind::Fragment;
    int tier = 1;
    float radius = 10.0f;
};

float length(sf::Vector2f value)
{
    return std::sqrt(value.x * value.x + value.y * value.y);
}

sf::Vector2f normalized(sf::Vector2f value)
{
    const float size = length(value);
    if (size <= 0.001f)
    {
        return {0.0f, 0.0f};
    }

    return {value.x / size, value.y / size};
}

float distance(sf::Vector2f a, sf::Vector2f b)
{
    return length(a - b);
}

sf::Color shade(sf::Color color, float multiplier)
{
    return {
        static_cast<sf::Uint8>(std::clamp(color.r * multiplier, 0.0f, 255.0f)),
        static_cast<sf::Uint8>(std::clamp(color.g * multiplier, 0.0f, 255.0f)),
        static_cast<sf::Uint8>(std::clamp(color.b * multiplier, 0.0f, 255.0f)),
        color.a};
}

sf::Vector2f clampToWorld(sf::Vector2f position)
{
    const sf::Vector2f fromCenter = position - kWorldCenter;
    const float fromCenterLength = length(fromCenter);
    if (fromCenterLength <= kWorldRadius - 18.0f)
    {
        return position;
    }

    return kWorldCenter + normalized(fromCenter) * (kWorldRadius - 18.0f);
}

sf::ConvexShape makeBiomeWedge(float startDegrees, float endDegrees, sf::Color color)
{
    constexpr int steps = 24;
    sf::ConvexShape shape;
    shape.setPointCount(steps + 2);
    shape.setPoint(0, kWorldCenter);

    for (int i = 0; i <= steps; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        const float degrees = startDegrees + (endDegrees - startDegrees) * t;
        const float radians = degrees * kPi / 180.0f;
        shape.setPoint(
            static_cast<std::size_t>(i + 1),
            kWorldCenter + sf::Vector2f{std::cos(radians), std::sin(radians)} * kWorldRadius);
    }

    shape.setFillColor(color);
    return shape;
}

void drawBar(sf::RenderWindow& window, sf::Vector2f position, sf::Vector2f size, float ratio, sf::Color fill)
{
    sf::RectangleShape back(size);
    back.setPosition(position);
    back.setFillColor(sf::Color(22, 24, 28));
    back.setOutlineColor(sf::Color(6, 7, 9));
    back.setOutlineThickness(2.0f);
    window.draw(back);

    sf::RectangleShape front({size.x * std::clamp(ratio, 0.0f, 1.0f), size.y});
    front.setPosition(position);
    front.setFillColor(fill);
    window.draw(front);
}

void damageEnemy(std::vector<Enemy>& enemies, std::size_t index, float damage)
{
    if (index < enemies.size())
    {
        enemies[index].hp -= damage;
    }
}

class Game
{
public:
    void run()
    {
        sf::RenderWindow window(sf::VideoMode(kWindowWidth, kWindowHeight), "The Circle");
        window.setFramerateLimit(60);

        sf::Clock clock;
        float titleTimer = 0.0f;
        setMenuTitle(window);

        while (window.isOpen())
        {
            const float dt = std::min(clock.restart().asSeconds(), 0.033f);
            handleEvents(window);

            if (state_ == GameState::Playing)
            {
                update(window, dt);
            }

            titleTimer -= dt;
            if (titleTimer <= 0.0f)
            {
                updateTitle(window);
                titleTimer = 0.2f;
            }

            draw(window);
        }
    }

private:
    GameState state_ = GameState::Menu;
    Player player_{};
    int biomeIndex_ = 0;
    bool bossRoom_ = false;
    bool bossDefeated_ = false;
    float spawnTimer_ = 0.0f;
    float runTime_ = 0.0f;
    std::mt19937 rng_{std::random_device{}()};
    std::vector<Enemy> enemies_;
    std::vector<Projectile> projectiles_;
    std::vector<Loot> loot_;

    void handleEvents(sf::RenderWindow& window)
    {
        sf::Event event{};
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                {
                    window.close();
                }

                if (event.key.code == sf::Keyboard::Enter &&
                    (state_ == GameState::Menu || state_ == GameState::Dead || state_ == GameState::Victory))
                {
                    startRun();
                }

                if (state_ == GameState::Playing && event.key.code == sf::Keyboard::Tab)
                {
                    player_.weapon = player_.weapon == WeaponMode::Sword ? WeaponMode::Bow : WeaponMode::Sword;
                }
            }
        }
    }

    void startRun()
    {
        state_ = GameState::Playing;
        player_ = Player{};
        biomeIndex_ = 0;
        bossRoom_ = false;
        bossDefeated_ = false;
        spawnTimer_ = 0.0f;
        runTime_ = 0.0f;
        enemies_.clear();
        projectiles_.clear();
        loot_.clear();
        spawnWave(6);
    }

    float playerDamage() const
    {
        const float base = player_.weapon == WeaponMode::Sword ? 12.0f : 15.0f;
        return base + static_cast<float>(player_.gearTier - 1) * 9.0f;
    }

    float playerArmor() const
    {
        return static_cast<float>(player_.gearTier - 1) * 3.0f;
    }

    void update(sf::RenderWindow& window, float dt)
    {
        runTime_ += dt;
        player_.attackCooldown = std::max(0.0f, player_.attackCooldown - dt);

        updatePlayer(window, dt);
        updateEnemies(dt);
        updateProjectiles(dt);
        updateLoot();
        removeDeadEnemies();
        updateSpawning(dt);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::E))
        {
            tryUseGate();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) && player_.potionCharges > 0 && player_.hp < player_.maxHp)
        {
            --player_.potionCharges;
            player_.hp = std::min(player_.maxHp, player_.hp + 45.0f);
        }

        if (player_.hp <= 0.0f)
        {
            state_ = GameState::Dead;
        }
    }

    void updatePlayer(sf::RenderWindow& window, float dt)
    {
        sf::Vector2f move{0.0f, 0.0f};
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        {
            move.x -= 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        {
            move.x += 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        {
            move.y -= 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        {
            move.y += 1.0f;
        }

        if (length(move) > 0.0f)
        {
            player_.position += normalized(move) * player_.speed * dt;
            player_.position = clampToWorld(player_.position);
        }

        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && player_.attackCooldown <= 0.0f)
        {
            const sf::Vector2f target = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            const sf::Vector2f direction = normalized(target - player_.position);
            attack(direction);
        }
    }

    void attack(sf::Vector2f direction)
    {
        if (length(direction) <= 0.0f)
        {
            direction = {1.0f, 0.0f};
        }

        if (player_.weapon == WeaponMode::Sword)
        {
            player_.attackCooldown = 0.38f;
            const sf::Vector2f strikeCenter = player_.position + direction * 42.0f;
            for (std::size_t i = 0; i < enemies_.size(); ++i)
            {
                if (distance(enemies_[i].position, strikeCenter) <= enemies_[i].radius + 46.0f)
                {
                    damageEnemy(enemies_, i, playerDamage());
                }
            }
        }
        else
        {
            player_.attackCooldown = 0.62f;
            projectiles_.push_back({player_.position + direction * 22.0f, direction * 540.0f, playerDamage(), 6.0f, true, 1.8f, sf::Color(230, 225, 145)});
        }
    }

    void updateEnemies(float dt)
    {
        for (Enemy& enemy : enemies_)
        {
            enemy.attackCooldown = std::max(0.0f, enemy.attackCooldown - dt);
            enemy.shootCooldown = std::max(0.0f, enemy.shootCooldown - dt);

            const sf::Vector2f toPlayer = player_.position - enemy.position;
            const float toPlayerDistance = length(toPlayer);
            sf::Vector2f direction = normalized(toPlayer);

            if (enemy.kind == EnemyKind::Ranged && toPlayerDistance < 150.0f)
            {
                direction *= -1.0f;
            }

            enemy.position += direction * enemy.speed * dt;
            enemy.position = clampToWorld(enemy.position);

            if ((enemy.kind == EnemyKind::Ranged || enemy.kind == EnemyKind::Boss) &&
                enemy.shootCooldown <= 0.0f && toPlayerDistance < 520.0f)
            {
                enemy.shootCooldown = enemy.kind == EnemyKind::Boss ? 0.72f : 1.3f;
                projectiles_.push_back({enemy.position, normalized(toPlayer) * (enemy.kind == EnemyKind::Boss ? 285.0f : 235.0f), enemy.damage, enemy.kind == EnemyKind::Boss ? 8.0f : 5.0f, false, 3.0f, shade(enemy.color, 1.35f)});
            }

            if (toPlayerDistance <= enemy.radius + 18.0f && enemy.attackCooldown <= 0.0f)
            {
                enemy.attackCooldown = enemy.kind == EnemyKind::Swarm ? 0.45f : 0.85f;
                applyDamageToPlayer(enemy.damage);
            }
        }
    }

    void updateProjectiles(float dt)
    {
        for (Projectile& projectile : projectiles_)
        {
            projectile.position += projectile.velocity * dt;
            projectile.lifetime -= dt;
        }

        for (Projectile& projectile : projectiles_)
        {
            if (projectile.lifetime <= 0.0f)
            {
                continue;
            }

            if (distance(projectile.position, kWorldCenter) > kWorldRadius)
            {
                projectile.lifetime = 0.0f;
                continue;
            }

            if (projectile.fromPlayer)
            {
                for (Enemy& enemy : enemies_)
                {
                    if (enemy.hp > 0.0f && distance(projectile.position, enemy.position) <= projectile.radius + enemy.radius)
                    {
                        enemy.hp -= projectile.damage;
                        projectile.lifetime = 0.0f;
                        break;
                    }
                }
            }
            else if (distance(projectile.position, player_.position) <= projectile.radius + 16.0f)
            {
                applyDamageToPlayer(projectile.damage);
                projectile.lifetime = 0.0f;
            }
        }

        projectiles_.erase(
            std::remove_if(projectiles_.begin(), projectiles_.end(), [](const Projectile& projectile) {
                return projectile.lifetime <= 0.0f;
            }),
            projectiles_.end());
    }

    void updateLoot()
    {
        for (Loot& item : loot_)
        {
            if (item.radius <= 0.0f || distance(item.position, player_.position) > item.radius + 18.0f)
            {
                continue;
            }

            if (item.kind == LootKind::Fragment)
            {
                ++player_.fragments;
            }
            else if (item.kind == LootKind::Gear)
            {
                if (item.tier > player_.gearTier)
                {
                    player_.gearTier = item.tier;
                    player_.maxHp += 10.0f + static_cast<float>(item.tier) * 4.0f;
                    player_.hp = std::min(player_.maxHp, player_.hp + 35.0f);
                }
            }
            else
            {
                player_.potionCharges = std::min(3, player_.potionCharges + 1);
            }

            item.radius = 0.0f;
        }

        loot_.erase(
            std::remove_if(loot_.begin(), loot_.end(), [](const Loot& item) {
                return item.radius <= 0.0f;
            }),
            loot_.end());
    }

    void removeDeadEnemies()
    {
        for (const Enemy& enemy : enemies_)
        {
            if (enemy.hp > 0.0f)
            {
                continue;
            }

            if (enemy.boss)
            {
                bossDefeated_ = true;
                state_ = GameState::Victory;
                continue;
            }

            ++player_.kills;
            if (player_.kills % 2 == 0)
            {
                player_.potionCharges = std::min(3, player_.potionCharges + 1);
            }

            std::uniform_real_distribution<float> chance(0.0f, 1.0f);
            loot_.push_back({enemy.position, LootKind::Fragment, kBiomes[biomeIndex_].tier, 9.0f});

            if (chance(rng_) < 0.28f)
            {
                loot_.push_back({enemy.position + sf::Vector2f{16.0f, -12.0f}, LootKind::Gear, kBiomes[biomeIndex_].tier, 11.0f});
            }

            if (chance(rng_) < 0.18f)
            {
                loot_.push_back({enemy.position + sf::Vector2f{-14.0f, 12.0f}, LootKind::PotionCharge, 1, 9.0f});
            }
        }

        enemies_.erase(
            std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& enemy) {
                return enemy.hp <= 0.0f;
            }),
            enemies_.end());
    }

    void updateSpawning(float dt)
    {
        if (bossRoom_)
        {
            return;
        }

        spawnTimer_ -= dt;
        const std::size_t targetEnemyCount = 5U + static_cast<std::size_t>(biomeIndex_) * 2U;
        if (spawnTimer_ <= 0.0f && enemies_.size() < targetEnemyCount)
        {
            spawnTimer_ = 1.15f;
            spawnEnemy();
        }
    }

    void applyDamageToPlayer(float incomingDamage)
    {
        float damage = std::max(1.0f, incomingDamage - playerArmor());
        if (player_.weapon == WeaponMode::Sword && sf::Mouse::isButtonPressed(sf::Mouse::Right))
        {
            damage *= 0.45f;
        }

        player_.hp -= damage;
    }

    sf::Vector2f randomSpawnPoint()
    {
        std::uniform_real_distribution<float> angle(0.0f, kPi * 2.0f);
        std::uniform_real_distribution<float> radius(70.0f, 170.0f);

        sf::Vector2f point{};
        do
        {
            const float a = angle(rng_);
            point = kBiomes[biomeIndex_].focus + sf::Vector2f{std::cos(a), std::sin(a)} * radius(rng_);
        } while (distance(point, player_.position) < 120.0f || distance(point, kWorldCenter) > kWorldRadius - 30.0f);

        return point;
    }

    void spawnWave(int count)
    {
        for (int i = 0; i < count; ++i)
        {
            spawnEnemy();
        }
    }

    void spawnEnemy()
    {
        std::uniform_int_distribution<int> kindRoll(0, 99);
        const int roll = kindRoll(rng_);
        Enemy enemy;

        if (roll < 44)
        {
            enemy.kind = EnemyKind::Melee;
        }
        else if (roll < 68)
        {
            enemy.kind = EnemyKind::Ranged;
        }
        else if (roll < 86)
        {
            enemy.kind = EnemyKind::Tank;
        }
        else
        {
            enemy.kind = EnemyKind::Swarm;
        }

        setupEnemy(enemy, kBiomes[biomeIndex_].tier);
        enemy.position = randomSpawnPoint();
        enemies_.push_back(enemy);
    }

    void setupEnemy(Enemy& enemy, int tier)
    {
        const float tierScale = static_cast<float>(tier);
        enemy.color = shade(kBiomes[biomeIndex_].color, 1.25f);

        switch (enemy.kind)
        {
            case EnemyKind::Melee:
                enemy.hp = enemy.maxHp = 20.0f + tierScale * 14.0f;
                enemy.speed = 82.0f + tierScale * 10.0f;
                enemy.damage = 7.0f + tierScale * 4.0f;
                enemy.radius = 14.0f;
                break;
            case EnemyKind::Ranged:
                enemy.hp = enemy.maxHp = 16.0f + tierScale * 10.0f;
                enemy.speed = 58.0f + tierScale * 6.0f;
                enemy.damage = 6.0f + tierScale * 4.0f;
                enemy.radius = 13.0f;
                enemy.color = shade(enemy.color, 1.45f);
                break;
            case EnemyKind::Tank:
                enemy.hp = enemy.maxHp = 48.0f + tierScale * 22.0f;
                enemy.speed = 38.0f + tierScale * 4.0f;
                enemy.damage = 12.0f + tierScale * 5.0f;
                enemy.radius = 22.0f;
                enemy.color = shade(enemy.color, 0.8f);
                break;
            case EnemyKind::Swarm:
                enemy.hp = enemy.maxHp = 9.0f + tierScale * 5.0f;
                enemy.speed = 130.0f + tierScale * 12.0f;
                enemy.damage = 4.0f + tierScale * 2.0f;
                enemy.radius = 9.0f;
                enemy.color = shade(enemy.color, 1.7f);
                break;
            case EnemyKind::Boss:
                break;
        }
    }

    void spawnBoss()
    {
        enemies_.clear();
        projectiles_.clear();
        loot_.clear();
        bossRoom_ = true;
        player_.position = kWorldCenter + sf::Vector2f{0.0f, 145.0f};

        Enemy boss;
        boss.kind = EnemyKind::Boss;
        boss.boss = true;
        boss.position = kWorldCenter;
        boss.hp = boss.maxHp = 360.0f;
        boss.speed = 66.0f;
        boss.damage = 18.0f;
        boss.radius = 34.0f;
        boss.color = shade(kBiomes[static_cast<std::size_t>(biomeIndex_)].color, 1.55f);
        enemies_.push_back(boss);
    }

    sf::Vector2f gatePosition() const
    {
        if (bossRoom_)
        {
            return kWorldCenter;
        }

        if (biomeIndex_ < 3)
        {
            return (kBiomes[static_cast<std::size_t>(biomeIndex_)].focus + kBiomes[static_cast<std::size_t>(biomeIndex_ + 1)].focus) * 0.5f;
        }

        return kWorldCenter;
    }

    void tryUseGate()
    {
        if (bossDefeated_ || player_.fragments < kFragmentsForGate || distance(player_.position, gatePosition()) > 72.0f)
        {
            return;
        }

        player_.fragments = 0;
        enemies_.clear();
        projectiles_.clear();
        loot_.clear();

        if (biomeIndex_ < 3)
        {
            ++biomeIndex_;
            player_.position = kBiomes[static_cast<std::size_t>(biomeIndex_)].focus;
            spawnWave(6 + biomeIndex_ * 2);
        }
        else
        {
            spawnBoss();
        }
    }

    void draw(sf::RenderWindow& window)
    {
        window.clear(sf::Color(10, 12, 15));
        drawWorld(window);

        if (state_ == GameState::Menu)
        {
            drawMenu(window);
        }
        else
        {
            drawGate(window);
            drawLoot(window);
            drawProjectiles(window);
            drawEnemies(window);
            drawPlayer(window);
            drawHud(window);

            if (state_ == GameState::Dead || state_ == GameState::Victory)
            {
                drawEndMarker(window);
            }
        }

        window.display();
    }

    void drawWorld(sf::RenderWindow& window)
    {
        window.draw(makeBiomeWedge(135.0f, 225.0f, shade(kBiomes[0].color, 0.55f)));
        window.draw(makeBiomeWedge(225.0f, 315.0f, shade(kBiomes[3].color, 0.55f)));
        window.draw(makeBiomeWedge(315.0f, 405.0f, shade(kBiomes[2].color, 0.55f)));
        window.draw(makeBiomeWedge(45.0f, 135.0f, shade(kBiomes[1].color, 0.55f)));

        sf::CircleShape border(kWorldRadius);
        border.setOrigin(kWorldRadius, kWorldRadius);
        border.setPosition(kWorldCenter);
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(sf::Color(210, 210, 190, 120));
        border.setOutlineThickness(5.0f);
        window.draw(border);

        sf::CircleShape bossRoom(64.0f);
        bossRoom.setOrigin(64.0f, 64.0f);
        bossRoom.setPosition(kWorldCenter);
        bossRoom.setFillColor(sf::Color(28, 24, 32, 225));
        bossRoom.setOutlineColor(sf::Color(210, 185, 90, 170));
        bossRoom.setOutlineThickness(3.0f);
        window.draw(bossRoom);
    }

    void drawMenu(sf::RenderWindow& window)
    {
        sf::CircleShape ring(92.0f);
        ring.setOrigin(92.0f, 92.0f);
        ring.setPosition(kWorldCenter);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(sf::Color(225, 220, 180));
        ring.setOutlineThickness(8.0f);
        window.draw(ring);

        sf::RectangleShape sword({18.0f, 150.0f});
        sword.setOrigin(9.0f, 75.0f);
        sword.setPosition(kWorldCenter);
        sword.setRotation(45.0f);
        sword.setFillColor(sf::Color(190, 205, 220));
        window.draw(sword);
    }

    void drawGate(sf::RenderWindow& window)
    {
        if (state_ != GameState::Playing || bossRoom_)
        {
            return;
        }

        sf::RectangleShape gate({46.0f, 46.0f});
        gate.setOrigin(23.0f, 23.0f);
        gate.setPosition(gatePosition());
        gate.setRotation(45.0f);
        gate.setFillColor(player_.fragments >= kFragmentsForGate ? sf::Color(230, 205, 70) : sf::Color(80, 80, 84));
        gate.setOutlineColor(sf::Color(20, 20, 18));
        gate.setOutlineThickness(3.0f);
        window.draw(gate);
    }

    void drawPlayer(sf::RenderWindow& window)
    {
        sf::RectangleShape body({32.0f, 32.0f});
        body.setOrigin(16.0f, 16.0f);
        body.setPosition(player_.position);
        body.setFillColor(player_.weapon == WeaponMode::Sword ? sf::Color(74, 220, 160) : sf::Color(95, 185, 245));
        body.setOutlineColor(sf::Mouse::isButtonPressed(sf::Mouse::Right) ? sf::Color(240, 240, 210) : sf::Color(8, 10, 12));
        body.setOutlineThickness(3.0f);
        window.draw(body);

        sf::RectangleShape weapon(player_.weapon == WeaponMode::Sword ? sf::Vector2f{9.0f, 34.0f} : sf::Vector2f{34.0f, 7.0f});
        weapon.setOrigin(weapon.getSize() * 0.5f);
        weapon.setPosition(player_.position + sf::Vector2f{24.0f, -18.0f});
        weapon.setFillColor(sf::Color(230, 225, 145));
        window.draw(weapon);
    }

    void drawEnemies(sf::RenderWindow& window)
    {
        for (const Enemy& enemy : enemies_)
        {
            sf::RectangleShape body({enemy.radius * 2.0f, enemy.radius * 2.0f});
            body.setOrigin(enemy.radius, enemy.radius);
            body.setPosition(enemy.position);
            body.setFillColor(enemy.color);
            body.setOutlineColor(enemy.boss ? sf::Color(255, 230, 120) : sf::Color(9, 9, 12));
            body.setOutlineThickness(enemy.boss ? 4.0f : 2.0f);
            window.draw(body);

            drawBar(window, enemy.position + sf::Vector2f{-enemy.radius, -enemy.radius - 9.0f}, {enemy.radius * 2.0f, 4.0f}, enemy.hp / enemy.maxHp, sf::Color(210, 60, 60));
        }
    }

    void drawProjectiles(sf::RenderWindow& window)
    {
        for (const Projectile& projectile : projectiles_)
        {
            sf::CircleShape shape(projectile.radius);
            shape.setOrigin(projectile.radius, projectile.radius);
            shape.setPosition(projectile.position);
            shape.setFillColor(projectile.color);
            window.draw(shape);
        }
    }

    void drawLoot(sf::RenderWindow& window)
    {
        for (const Loot& item : loot_)
        {
            sf::RectangleShape shape({item.radius * 1.6f, item.radius * 1.6f});
            shape.setOrigin(item.radius * 0.8f, item.radius * 0.8f);
            shape.setPosition(item.position);
            shape.setRotation(item.kind == LootKind::Fragment ? 45.0f : 0.0f);

            if (item.kind == LootKind::Fragment)
            {
                shape.setFillColor(sf::Color(245, 210, 60));
            }
            else if (item.kind == LootKind::Gear)
            {
                shape.setFillColor(sf::Color(90, 220, 230));
            }
            else
            {
                shape.setFillColor(sf::Color(80, 230, 115));
            }

            window.draw(shape);
        }
    }

    void drawHud(sf::RenderWindow& window)
    {
        drawBar(window, {26.0f, 24.0f}, {250.0f, 18.0f}, player_.hp / player_.maxHp, sf::Color(210, 55, 64));

        for (int i = 0; i < kFragmentsForGate; ++i)
        {
            sf::RectangleShape fragment({16.0f, 16.0f});
            fragment.setOrigin(8.0f, 8.0f);
            fragment.setRotation(45.0f);
            fragment.setPosition({38.0f + static_cast<float>(i) * 28.0f, 68.0f});
            fragment.setFillColor(i < player_.fragments ? sf::Color(245, 210, 60) : sf::Color(48, 50, 54));
            window.draw(fragment);
        }

        for (int i = 0; i < 3; ++i)
        {
            sf::CircleShape potion(9.0f);
            potion.setOrigin(9.0f, 9.0f);
            potion.setPosition({210.0f + static_cast<float>(i) * 26.0f, 68.0f});
            potion.setFillColor(i < player_.potionCharges ? sf::Color(80, 230, 115) : sf::Color(48, 50, 54));
            window.draw(potion);
        }

        sf::RectangleShape tier({18.0f + static_cast<float>(player_.gearTier) * 12.0f, 12.0f});
        tier.setPosition({26.0f, 91.0f});
        tier.setFillColor(sf::Color(90, 220, 230));
        window.draw(tier);
    }

    void drawEndMarker(sf::RenderWindow& window)
    {
        sf::CircleShape marker(84.0f);
        marker.setOrigin(84.0f, 84.0f);
        marker.setPosition(kWorldCenter);
        marker.setFillColor(state_ == GameState::Victory ? sf::Color(230, 205, 70, 190) : sf::Color(150, 30, 42, 190));
        marker.setOutlineColor(sf::Color(245, 240, 220));
        marker.setOutlineThickness(5.0f);
        window.draw(marker);
    }

    void setMenuTitle(sf::RenderWindow& window)
    {
        window.setTitle("The Circle | Enter start | WASD move | LMB attack | RMB block | Q potion | E gate | Tab weapon");
    }

    void updateTitle(sf::RenderWindow& window)
    {
        if (state_ == GameState::Menu)
        {
            setMenuTitle(window);
            return;
        }

        if (state_ == GameState::Dead)
        {
            window.setTitle("The Circle | Death ends the run | Press Enter to restart");
            return;
        }

        if (state_ == GameState::Victory)
        {
            window.setTitle("The Circle | Boss defeated | Press Enter for a new run");
            return;
        }

        std::ostringstream title;
        title << "The Circle | "
              << (bossRoom_ ? "Boss Room" : kBiomes[static_cast<std::size_t>(biomeIndex_)].name)
              << " | HP " << static_cast<int>(std::max(0.0f, player_.hp)) << "/" << static_cast<int>(player_.maxHp)
              << " | Key " << player_.fragments << "/" << kFragmentsForGate
              << " | Tier " << player_.gearTier
              << " | Potions " << player_.potionCharges
              << " | " << (player_.weapon == WeaponMode::Sword ? "Sword" : "Bow");
        window.setTitle(title.str());
    }
};
}

int main()
{
    Game game;
    game.run();
    return 0;
}
