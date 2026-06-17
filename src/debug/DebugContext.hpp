#pragma once

#ifdef TC_DEBUG

#include <entt/entt.hpp>

namespace tc {

class World;
class Game;
class PlayState;
class RenderSystem;

// Bundles everything the debug panels need to read/mutate live game state.
struct DebugContext {
    entt::registry& registry;
    entt::entity player;
    World& world;
    Game& game;
    PlayState& playState;
    RenderSystem& renderSystem;
    float lastDt = 0.0f;
};

} // namespace tc

#endif // TC_DEBUG
