#include "sprite/SpriteSheetLoader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace tc::sprite {

namespace {

nlohmann::json loadJsonFile(const std::string& jsonPath)
{
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        throw std::runtime_error("[SpriteSheetLoader] Failed to open JSON file: " + jsonPath);
    }

    nlohmann::json parsed;
    try {
        file >> parsed;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("[SpriteSheetLoader] Malformed JSON in " + jsonPath + ": " + e.what());
    }
    return parsed;
}

} // namespace

std::unordered_map<std::string, std::shared_ptr<SpriteSheetAsset>> SpriteSheetLoader::cache;

std::string SpriteSheetLoader::cacheKey(const std::string& pngPath, const std::string& jsonPath)
{
    return pngPath + "|" + jsonPath;
}

void SpriteSheetLoader::clearCache()
{
    cache.clear();
}

std::shared_ptr<SpriteSheetAsset> SpriteSheetLoader::load(const std::string& pngPath, const std::string& jsonPath)
{
    const std::string key = cacheKey(pngPath, jsonPath);
    auto cached = cache.find(key);
    if (cached != cache.end()) {
        return cached->second;
    }

    const nlohmann::json root = loadJsonFile(jsonPath);

    if (!root.contains("spritesheet")) {
        throw std::runtime_error("[SpriteSheetLoader] " + jsonPath + " is missing \"spritesheet\"");
    }
    const nlohmann::json& sheet = root.at("spritesheet");

    if (!sheet.contains("cell_size")) {
        throw std::runtime_error("[SpriteSheetLoader] " + jsonPath + " is missing \"spritesheet.cell_size\"");
    }
    if (!sheet.contains("rows")) {
        throw std::runtime_error("[SpriteSheetLoader] " + jsonPath + " is missing \"spritesheet.rows\"");
    }

    const int cellWidth = sheet.at("cell_size").at("width").get<int>();
    const int cellHeight = sheet.at("cell_size").at("height").get<int>();

    auto asset = std::make_shared<SpriteSheetAsset>();

    if (!asset->texture.loadFromFile(pngPath)) {
        throw std::runtime_error("[SpriteSheetLoader] Failed to load texture: " + pngPath);
    }

    for (const nlohmann::json& row : sheet.at("rows")) {
        const int rowIndex = row.at("row").get<int>();
        const std::string type = row.at("type").get<std::string>();
        const int frameCount = row.at("frame_count").get<int>();

        std::vector<sf::IntRect> frames;
        frames.reserve(static_cast<std::size_t>(frameCount));
        for (int col = 0; col < frameCount; ++col) {
            frames.emplace_back(col * cellWidth, rowIndex * cellHeight, cellWidth, cellHeight);
        }

        if (type == "rotations") {
            const auto& directions = row.at("directions");
            if (static_cast<int>(directions.size()) != frameCount) {
                throw std::runtime_error(
                    "[SpriteSheetLoader] " + jsonPath + " rotations row has directions/frame_count mismatch");
            }
            for (int i = 0; i < frameCount; ++i) {
                const Facing facing = facingFromString(directions.at(static_cast<std::size_t>(i)).get<std::string>());
                asset->idleRotationFrames[static_cast<std::size_t>(facing)] = frames[static_cast<std::size_t>(i)];
            }
        } else if (type == "animation") {
            const std::string animationName = row.at("animation").get<std::string>();
            const Facing facing = facingFromString(row.at("direction").get<std::string>());
            AnimationFrameData data;
            data.frames = std::move(frames);
            asset->animations[animationName][facing] = std::move(data);
        } else {
            throw std::runtime_error("[SpriteSheetLoader] " + jsonPath + " has unknown row type: " + type);
        }
    }

    cache[key] = asset;
    return asset;
}

} // namespace tc::sprite
