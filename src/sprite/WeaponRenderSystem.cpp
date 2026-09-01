#include "sprite/WeaponRenderSystem.hpp"

#include <algorithm>
#include <optional>

#include "ecs/Components.hpp"
#include "sprite/SpriteTypes.hpp"
#include "sprite/WeaponTypes.hpp"

namespace tc::sprite {

namespace {

const SocketKeyframe* findKeyframe(const std::vector<SocketKeyframe>& keyframes, int frameIndex)
{
    const auto it = std::find_if(keyframes.begin(), keyframes.end(),
        [frameIndex](const SocketKeyframe& kf) { return kf.frameIndex == frameIndex; });
    return it == keyframes.end() ? nullptr : &(*it);
}

std::optional<sf::Vector2f> findHandSocket(const HandSocketData& sockets, Facing direction, int frameIndex)
{
    const auto it = sockets.socketPerFrame.find(direction);
    if (it == sockets.socketPerFrame.end()) {
        return std::nullopt;
    }
    if (frameIndex < 0 || frameIndex >= static_cast<int>(it->second.size())) {
        return std::nullopt;
    }
    return it->second[static_cast<std::size_t>(frameIndex)];
}

} // namespace

void WeaponRenderSystem::render(sf::RenderTarget& target, entt::registry& registry, bool drawFrontPass)
{
    auto view = registry.view<Position, AnimationState, WeaponAppearance>();
    for (auto entity : view) {
        const auto& pos = view.get<Position>(entity);
        const auto& state = view.get<AnimationState>(entity);
        const auto& weapon = view.get<WeaponAppearance>(entity);

        if (!weapon.weapon || !weapon.weapon->curve || !weapon.handSockets) {
            continue;
        }

        const FacingMirror mirror = resolveMirror(state.facing);

        const std::vector<SocketKeyframe>& keyframes = curveForDirection(*weapon.weapon->curve, mirror.source);
        const SocketKeyframe* keyframe = findKeyframe(keyframes, state.currentFrame);
        if (!keyframe || keyframe->drawInFrontOfBody != drawFrontPass) {
            continue;
        }

        const std::optional<sf::Vector2f> handSocket = findHandSocket(*weapon.handSockets, mirror.source, state.currentFrame);
        if (!handSocket) {
            continue;
        }

        sf::Vector2f socket = *handSocket;
        sf::Vector2f offset = keyframe->offset;
        float rotationDeg = keyframe->rotationDeg;
        if (mirror.mirrored) {
            socket.x = -socket.x;
            offset.x = -offset.x;
            rotationDeg = -rotationDeg;
        }

        const sf::Vector2f finalPos = sf::Vector2f(pos.x, pos.y) + socket + offset;

        sf::Sprite sprite;
        sprite.setTexture(weapon.weapon->bladeTexture);
        const sf::Vector2u textureSize = weapon.weapon->bladeTexture.getSize();
        sprite.setOrigin(textureSize.x / 2.0f, textureSize.y / 2.0f);
        sprite.setRotation(rotationDeg);
        sprite.setScale(mirror.mirrored ? -1.0f : 1.0f, 1.0f);
        sprite.setPosition(finalPos);
        target.draw(sprite);
    }
}

} // namespace tc::sprite
