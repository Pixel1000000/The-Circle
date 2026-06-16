#include "ecs/ItemUpgrader.hpp"

#include <algorithm>

namespace tc {

namespace {

void setTier(Equipment& equipment, int slot, int tier)
{
    switch (slot) {
    case 0: equipment.helmetTier = tier; break;
    case 1: equipment.chestTier = tier; break;
    case 2: equipment.leggingsTier = tier; break;
    case 3: equipment.weaponTier = tier; break;
    default: break;
    }
}

void setElement(Equipment& equipment, int slot, Element element, float percent)
{
    switch (slot) {
    case 0:
        equipment.helmetElement = element;
        equipment.helmetElementPercent = percent;
        break;
    case 1:
        equipment.chestElement = element;
        equipment.chestElementPercent = percent;
        break;
    case 2:
        equipment.leggingsElement = element;
        equipment.leggingsElementPercent = percent;
        break;
    case 3:
        equipment.weaponElement = element;
        equipment.weaponElementPercent = percent;
        break;
    default: break;
    }
}

} // namespace

void ItemUpgrader::takeNew(Equipment& equipment, int slot, int tier, Element element, float percent)
{
    setTier(equipment, slot, tier);
    setElement(equipment, slot, element, percent);

    if (slot == 3 && equipment.weaponType == Equipment::NONE) {
        equipment.weaponType = Equipment::SWORD;
    }
}

void ItemUpgrader::upgradeCurrent(Equipment& equipment, int slot, int tier, Element element, float percent)
{
    setTier(equipment, slot, std::max(getTier(equipment, slot), tier));

    const auto [currentElement, currentPercent] = getElement(equipment, slot);
    const Element resultElement = currentElement == Element::NONE ? element : currentElement;
    setElement(equipment, slot, resultElement, currentPercent + percent * 0.5f);

    if (slot == 3 && equipment.weaponType == Equipment::NONE) {
        equipment.weaponType = Equipment::SWORD;
    }
}

int ItemUpgrader::getTier(const Equipment& equipment, int slot)
{
    switch (slot) {
    case 0: return equipment.helmetTier;
    case 1: return equipment.chestTier;
    case 2: return equipment.leggingsTier;
    case 3: return equipment.weaponTier;
    default: return 0;
    }
}

std::pair<Element, float> ItemUpgrader::getElement(const Equipment& equipment, int slot)
{
    switch (slot) {
    case 0: return {equipment.helmetElement, equipment.helmetElementPercent};
    case 1: return {equipment.chestElement, equipment.chestElementPercent};
    case 2: return {equipment.leggingsElement, equipment.leggingsElementPercent};
    case 3: return {equipment.weaponElement, equipment.weaponElementPercent};
    default: return {Element::NONE, 0.0f};
    }
}

bool ItemUpgrader::getAutoUpgrade(const Equipment& equipment, int slot)
{
    switch (slot) {
    case 0: return equipment.autoUpgradeHelmet;
    case 1: return equipment.autoUpgradeChest;
    case 2: return equipment.autoUpgradeLeggings;
    case 3: return equipment.autoUpgradeWeapon;
    default: return false;
    }
}

void ItemUpgrader::setAutoUpgrade(Equipment& equipment, int slot, bool value)
{
    switch (slot) {
    case 0: equipment.autoUpgradeHelmet = value; break;
    case 1: equipment.autoUpgradeChest = value; break;
    case 2: equipment.autoUpgradeLeggings = value; break;
    case 3: equipment.autoUpgradeWeapon = value; break;
    default: break;
    }
}

} // namespace tc
