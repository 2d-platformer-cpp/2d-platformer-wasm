/// @file EntityManager.hpp
/// Owns all entities in the current level, drives update/render/collision passes,
/// tracks player death, and garbage-collects destroyed entities.

#pragma once

#include <vector>
#include <memory>
#include "Common/Types.hpp"
#include "Gameplay/Entity.hpp"

namespace Gameplay {

/// Configuration for level boundaries and wall behaviour.
struct LevelConfig {
    bool isLeftWallClamped  = true;  ///< If true, entities cannot move past X=0.
    bool isRightWallClamped = true;  ///< If true, entities cannot move past levelWidth - width.
    float levelWidth        = Common::DEFAULT_LEVEL_WIDTH;  ///< Total width of the level in logical pixels.
};

/// Owns all entities in the current level.
/// Drives update/render passes, handles collision detection (AABB),
/// tracks player death, and garbage-collects destroyed entities.
class EntityManager {
public:
    /// Default constructor; entities are added via addEntity() after construction.
    EntityManager() = default;

    /// Advances all entities by one timestep: applies physics, clamps to level bounds,
    /// removes destroyed entities.
    /// @param deltaTime  Fixed time step in seconds.
    /// @param input      Current frame input snapshot.
    void update(float deltaTime, const Common::InputState &input);

    /// Sorts all living entities by zIndex and pushes their RenderCommands.
    /// @param commands  Render command list to extend.
    void render(std::vector<Common::RenderCommand> &commands);

    /// Adds an entity, assigns it a unique ID, and takes ownership.
    /// @param entity  Shared pointer to the entity.
    void addEntity(std::shared_ptr<Entity> entity);

    /// Removes all entities immediately.
    void clear();

    /// Finds the first Player entity in the entity list.
    /// @return Shared pointer to the Player, or nullptr if none exists.
    std::shared_ptr<Player> getPlayer() const;

    /// Returns a const reference to the full entity list.
    const std::vector<std::shared_ptr<Entity>> &getEntities() const { return m_entities; }

    /// Performs AABB collision detection between the player and all other solid/trigger/hazard entities.
    /// Resolves solid overlaps via pure overlap-magnitude comparison (no velocity overrides).
    /// Returns exit information if the player overlaps an exit trigger.
    /// @return LevelExit info if the player triggered an exit, nullopt otherwise.
    std::optional<LevelExit> checkCollisions();

    /// Per-level configuration (walls, width).
    LevelConfig levelConfig;

    /// Whether the player has died this frame (from hazard touch or falling off-screen).
    bool isPlayerDead() const { return m_deathByHazard || m_deathByFall; }
    /// Whether the player was killed by a hazard.
    bool wasDeathByHazard() const { return m_deathByHazard; }
    /// Whether the player died by falling below the screen.
    bool wasDeathByFall() const { return m_deathByFall; }
    /// Resets all death flags (call after processing a death).
    void resetDeathFlag() {
        m_deathByHazard = false;
        m_deathByFall = false;
    }

private:
    std::vector<std::shared_ptr<Entity>> m_entities;  ///< All entities in the level.
    int m_nextEntityID = 0;             ///< Monotonic ID counter for new entities.
    bool m_deathByHazard = false;       ///< True when a hazard kills the player.
    bool m_deathByFall = false;         ///< True when the player falls below SCREEN_HEIGHT.
};

} // namespace Gameplay
