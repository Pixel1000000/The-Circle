#pragma once

#include <utility>

#include "ecs/Components.hpp"

namespace tc {

// Applies the "take new item" / "upgrade current item" choice (from
// ItemChoiceScreen, or automatically when Equipment.autoUpgrade* is set) to
// the player's Equipment. Slot indices: 0 = helmet, 1 = chest, 2 = leggings,
// 3 = weapon.
namespace ItemUpgrader {

// Replaces the slot's tier and elemental affinity with the dropped item.
void takeNew(Equipment& equipment, int slot, int tier, Element element, float percent);

// Raises the slot's tier to `tier` (never lowers it) and adds half the
// dropped item's elemental percent to the slot's existing affinity, adopting
// the dropped element if the slot currently has none.
void upgradeCurrent(Equipment& equipment, int slot, int tier, Element element, float percent);

// Current tier equipped in `slot` (0 if empty).
int getTier(const Equipment& equipment, int slot);

// Current elemental affinity equipped in `slot` (Element::NONE, 0.f if none).
std::pair<Element, float> getElement(const Equipment& equipment, int slot);

// Whether drops for `slot` should be applied via upgradeCurrent() without
// showing ItemChoiceScreen.
bool getAutoUpgrade(const Equipment& equipment, int slot);
void setAutoUpgrade(Equipment& equipment, int slot, bool value);

} // namespace ItemUpgrader

} // namespace tc
