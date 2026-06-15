#pragma once

#include <entt/entt.hpp>

#include "ecs/Components.hpp"

namespace tc {

// Shared helpers for applying elemental on-hit effects (NatureStack, FireBurn,
// IceChill + bonus damage, DecayEffect) and SLOW, respecting ElementalResist.
namespace ElementalUtils {

// Applies the on-hit effect for `element`/`percent` (from the attacker's
// weapon) to `target`. `rawDamage` is the attacker's pre-mitigation damage,
// used by ICE's bonus-damage calculation.
void applyOnHit(entt::registry& registry, entt::entity target, Element element, float percent, int rawDamage);

// Applies StatusEffect::SLOW for `duration`, reduced by the target's
// ElementalResist::slowResist (1.0 = full immunity, applies no effect at all).
void applySlow(entt::registry& registry, entt::entity target, float duration);

} // namespace ElementalUtils

} // namespace tc
