/// @file Entity.hpp
/// Entity class hierarchy: Entity base, Player, StaticObject, and BackgroundLayer.
/// Each entity owns optional component slots (sprite, physics, collider, etc.).

#pragma once

#include <vector>
#include <optional>
#include "Common/Types.hpp"
#include "Gameplay/Components.hpp"

namespace Gameplay {

/// Abstract base class for all in-game objects.
/// Provides optional component slots (sprite, physics, collider, exits, controls, parallax).
/// Subclasses override update() and render() to define their behavior and appearance.
class Entity {
public:
    /// Default constructor.
    Entity() = default;
    /// Virtual destructor for polymorphic cleanup.
    virtual ~Entity() = default;
    Entity(const Entity &) = delete;
    Entity &operator=(const Entity &) = delete;

    /// Per-frame logic hook. Called each fixed timestep.
    /// @param deltaTime  Time step in seconds.
    virtual void update(float deltaTime) = 0;

    /// Appends RenderCommands for this entity to the list.
    /// @param commands  Render command list to extend.
    virtual void render(std::vector<Common::RenderCommand> &commands) = 0;

    // --- Component slots (optional — only engaged for relevant entity types) ---
    Transform transform;                          ///< Position, size, draw order.
    std::optional<Sprite> sprite;                 ///< Visual appearance (used by all entities).
    std::optional<Physics> physics;               ///< Movement physics (used by Player).
    std::optional<Collider> collider;             ///< Hitbox for collisions (used by Player and StaticObject).
    std::optional<LevelExit> exitEast;            ///< East-bound level exit (used by StaticObject exits).
    std::optional<LevelExit> exitWest;            ///< West-bound level exit (used by StaticObject exits).
    std::optional<PlayerControl> playerControl;   ///< Player movement parameters (used by Player).
    std::optional<Parallax> parallax;             ///< Parallax scroll factor (used by BackgroundLayer).

    int id = -1;              ///< Unique entity identifier assigned by EntityManager.
    bool isDestroyed = false; ///< If true, entity will be removed on next EntityManager::update.
};

/// Playable character entity.
/// Reads keyboard input for movement/jump, applies physics, and renders as TEX_PLAYER.
class Player : public Entity {
public:
    /// Constructs a Player with default Transform, Physics, Collider, Sprite, and PlayerControl components.
    Player();
    /// Applies gravity and integrates velocity into position.
    /// @param deltaTime  Fixed time step in seconds.
    void update(float deltaTime) override;
    /// Pushes a RenderCommand with TEX_PLAYER.
    /// @param commands  Render command list to extend.
    void render(std::vector<Common::RenderCommand> &commands) override;
    /// Reads horizontal input, friction/acceleration, jump action.
    /// @param input      Current frame input snapshot.
    /// @param deltaTime  Time step in seconds.
    void handleInput(const Common::InputState &input, float deltaTime);
};

/// Static world geometry: platforms, floors, walls, hazards, exit triggers.
/// Does not move; may have collider and exit components.
class StaticObject : public Entity {
public:
    /// Default constructor.
    StaticObject() = default;
    /// Static object has no per-frame logic.
    /// @param /*deltaTime*/  Ignored.
    void update(float /*deltaTime*/) override {}
    /// Pushes a RenderCommand based on the entity's Sprite textureID.
    /// Fallback color is derived from the collider type (solid=green, hazard=orange/red).
    /// @param commands  Render command list to extend.
    void render(std::vector<Common::RenderCommand> &commands) override;
};

/// Scrolling background plane with parallax effect.
/// Positioned at (0, 0) spanning the viewport; scroll speed determined by Parallax::factor.
class BackgroundLayer : public Entity {
public:
    /// Default constructor.
    BackgroundLayer() = default;
    /// Parallax background has no per-frame logic.
    /// @param /*deltaTime*/  Ignored.
    void update(float /*deltaTime*/) override {}
    /// Pushes a RenderCommand with the entity's textureID and parallax scroll factor.
    /// @param commands  Render command list to extend.
    void render(std::vector<Common::RenderCommand> &commands) override;
};

} // namespace Gameplay
