#include "core/Localization.hpp"

#include <fstream>
#include <iostream>

namespace tc {

void Localization::load(const std::string& langCode)
{
    const std::string path = "assets/lang/" + langCode + ".json";
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Localization] Could not open " << path << "\n";
        return;
    }

    try {
        file >> strings;
        currentLanguage = langCode;
    } catch (const std::exception& e) {
        std::cerr << "[Localization] Failed to parse " << path << ": " << e.what() << "\n";
    }
}

std::string Localization::get(const std::string& key) const
{
    if (strings.contains(key)) {
        return strings.at(key).get<std::string>();
    }
    std::cerr << "[Localization] Missing key: " << key << "\n";
    return key;
}

const std::string& Localization::getCurrentLanguage() const
{
    return currentLanguage;
}

} // namespace tc
