#include "sprite/RenderSystem.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <optional>
#include <set>
#include <tuple>

#include "ecs/Components.hpp"
#include "sprite/SpriteTypes.hpp"

namespace tc::sprite {

namespace {

std::optional<sf::IntRect> resolveFrameRect(const SpriteSheetAsset& sheet, const AnimationState& state)
{
    if (state.currentAnimation == "idle") {
        return sheet.idleRotationFrames[static_cast<std::size_t>(state.facing)];
    }

    const auto animIt = sheet.animations.find(state.currentAnimation);
    if (animIt == sheet.animations.end()) {
        return std::nullopt;
    }
    const auto dirIt = animIt->second.find(state.facing);
    if (dirIt == animIt->second.end()) {
        return std::nullopt;
    }
    const AnimationFrameData& data = dirIt->second;
    if (data.frames.empty()) {
        return std::nullopt;
    }
    const std::size_t frameIndex = static_cast<std::size_t>(
        std::min(state.currentFrame, static_cast<int>(data.frames.size()) - 1));
    return data.frames[frameIndex];
}

void drawLayer(sf::RenderTarget& target, const SpriteSheetAsset& sheet, const AnimationState& state, const Position& pos)
{
    const std::optional<sf::IntRect> rect = resolveFrameRect(sheet, state);
    if (!rect) {
        return;
    }

    sf::Sprite sprite;
    sprite.setTexture(sheet.texture);
    sprite.setTextureRect(*rect);
    sprite.setOrigin(rect->width / 2.0f, rect->height / 2.0f);
    sprite.setPosition(pos.x, pos.y);
    target.draw(sprite);
}

} // namespace

void RenderSystem::render(sf::RenderTarget& target, entt::registry& registry)
{
    // Dedup key for "layer has no data for this frame" warnings, so a
    // missing walk animation on an equipped item logs once per
    // (slot, animation, facing) instead of once per frame.
    static std::set<std::tuple<int, std::string, Facing>> warnedMissingLayers;

    auto view = registry.view<Position, AnimationState, BaseAppearance>();
    for (auto entity : view) {
        const auto& pos = view.get<Position>(entity);
        const auto& state = view.get<AnimationState>(entity);
        const auto& appearance = view.get<BaseAppearance>(entity);

        if (appearance.bodySheet) {
            drawLayer(target, *appearance.bodySheet, state, pos);
        }

        if (const auto* equipment = registry.try_get<EquipmentAppearance>(entity)) {
            static constexpr std::array<EquipSlot, 3> drawOrder = {EquipSlot::Legs, EquipSlot::Torso, EquipSlot::Head};
            for (EquipSlot slot : drawOrder) {
                const auto slotIndex = static_cast<std::size_t>(slot);
                const auto& layerSheet = equipment->layers[slotIndex];
                if (!layerSheet) {
                    continue;
                }

                if (resolveFrameRect(*layerSheet, state).has_value()) {
                    drawLayer(target, *layerSheet, state, pos);
                } else {
                    const auto key = std::make_tuple(static_cast<int>(slot), state.currentAnimation, state.facing);
                    if (warnedMissingLayers.insert(key).second) {
                        std::cerr << "[sprite::RenderSystem] Equipment slot " << slotIndex
                                   << " has no frame for animation \"" << state.currentAnimation << "\"\n";
                    }
                }
            }
        }
    }
}

} // namespace tc::sprite
