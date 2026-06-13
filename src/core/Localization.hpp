#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace tc {

// Loads assets/lang/<code>.json and exposes string lookups by key.
// Falls back to the key itself if it is missing from the loaded file.
class Localization {
public:
    void load(const std::string& langCode);
    std::string get(const std::string& key) const;
    const std::string& getCurrentLanguage() const;

private:
    nlohmann::json strings;
    std::string currentLanguage = "en";
};

} // namespace tc
