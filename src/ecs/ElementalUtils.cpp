#include "ecs/ElementalUtils.hpp"

#include <algorithm>
#include <cmath>

#include "config/ConfigLoader.hpp"

namespace tc {

namespace ElementalUtils {

void applySlow(entt::registry& registry, entt::entity target, float duration)
{
    float resist = 0.0f;
    if (const auto* elementalResist = registry.try_get<ElementalResist>(target)) {
        resist = elementalResist->slowResist;
    }
    if (resist >= 1.0f) return;

    const float effectiveDuration = duration * (1.0f - resist);
    if (effectiveDuration <= 0.0f) return;

    registry.emplace_or_replace<StatusEffect>(target, StatusEffect::SLOW, 0.0f, effectiveDuration, effectiveDuration);
}

void applyOnHit(entt::registry& registry, entt::entity target, Element element, float percent, int rawDamage)
{
    if (element == Element::NONE || percent <= 0.0f) return;

    auto* health = registry.try_get<Health>(target);
    if (!health || health->current <= 0) return;

    const auto& elementalConfig = ConfigLoader::get().getElementalConfig();

    switch (element) {
    case Element::NATURE: {
        if (auto* stack = registry.try_get<NatureStack>(target)) {
            stack->dpsMultiplier *= (1.0f + percent);
            ++stack->stacks;
            stack->duration = elementalConfig.natureDuration;
            stack->timer = elementalConfig.natureDuration;
        } else {
            registry.emplace<NatureStack>(target, 1.0f + percent, elementalConfig.natureDuration, elementalConfig.natureDuration, 1, 0.0f);
        }
        break;
    }
    case Element::FIRE: {
        const float dps = elementalConfig.fireBaseDps * (1.0f + percent);
        if (auto* burn = registry.try_get<FireBurn>(target)) {
            burn->dps = dps;
            burn->duration = elementalConfig.fireDuration;
            burn->timer = elementalConfig.fireDuration;
        } else {
            registry.emplace<FireBurn>(target, dps, elementalConfig.fireDuration, elementalConfig.fireDuration, 0.0f);
        }
        break;
    }
    case Element::ICE: {
        const int bonusDamage = static_cast<int>(std::round(static_cast<float>(rawDamage) * percent));
        if (bonusDamage > 0) {
            health->current = std::max(health->current - bonusDamage, 0);
        }
        registry.emplace_or_replace<IceChill>(target, elementalConfig.iceSlowDuration);
        applySlow(registry, target, elementalConfig.iceSlowDuration);
        break;
    }
    case Element::DECAY: {
        if (auto* decay = registry.try_get<DecayEffect>(target)) {
            decay->duration = elementalConfig.decayDuration;
            decay->timer = elementalConfig.decayDuration;
        } else {
            float resist = 0.0f;
            if (const auto* elementalResist = registry.try_get<ElementalResist>(target)) {
                resist = elementalResist->decayResist;
            }
            const int reduction = static_cast<int>(std::round(static_cast<float>(health->max) * percent * (1.0f - resist)));
            if (reduction > 0) {
                registry.emplace<DecayEffect>(target, reduction, elementalConfig.decayDuration, elementalConfig.decayDuration);
                health->max = std::max(health->max - reduction, 1);
                health->current = std::min(health->current, health->max);
            }
        }
        break;
    }
    default:
        break;
    }
}

} // namespace ElementalUtils

} // namespace tc
