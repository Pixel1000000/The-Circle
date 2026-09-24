#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "sprite/SpriteTypes.hpp"

namespace tc::sprite {

// Loads a PixelLab spritesheet (PNG atlas + JSON layout) into a
// SpriteSheetAsset. Never hardcodes frame coordinates: every rect is derived
// from the JSON at load time, since PixelLab recomputes the atlas layout
// whenever it is regenerated/extended.
class SpriteSheetLoader {
public:
    // Throws std::runtime_error if the PNG/JSON can't be read or the JSON is
    // missing the expected "spritesheet"/"rows"/"cell_size" structure.
    static std::shared_ptr<SpriteSheetAsset> load(const std::string& pngPath, const std::string& jsonPath);

    // Test/dev hook: drops every cached asset so subsequent load() calls
    // re-read from disk.
    static void clearCache();

private:
    static std::string cacheKey(const std::string& pngPath, const std::string& jsonPath);

    static std::unordered_map<std::string, std::shared_ptr<SpriteSheetAsset>> cache;
};

} // namespace tc::sprite
