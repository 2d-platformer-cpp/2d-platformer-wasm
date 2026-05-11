/// @file Components.hpp
/// Entity component structs used throughout the game: Transform, Sprite,
/// Physics, Collider, LevelExit, PlayerControl, and Parallax.

#pragma once

#include "Common/Constants.hpp"
#include <string>

namespace Gameplay {

/// Position, size, and draw order for any entity.
struct Transform {
    float x = 0.0f;                                   ///< World X position (left edge).
    float y = 0.0f;                                   ///< World Y position (top edge).
    float width = Common::DEFAULT_COLLIDER_WIDTH;      ///< Width in logical pixels.
    float height = Common::DEFAULT_COLLIDER_HEIGHT;    ///< Height in logical pixels.
    int zIndex = 0;                                    ///< Draw order (lower = drawn first / behind).
};

/// Associates an entity with a texture for rendering.
struct Sprite {
    Common::TextureID textureID = Common::TextureID::TEX_NONE;  ///< Which texture to draw (or fallback color).
};

/// Physics state for movable entities (player, enemies).
struct Physics {
    float velocityX = 0.0f;          ///< Horizontal velocity in px/s.
    float velocityY = 0.0f;          ///< Vertical velocity in px/s.
    float gravity   = Common::GRAVITY;       ///< Downward acceleration in px/s².
    float friction  = Common::FRICTION;      ///< Horizontal deceleration when no input.
    float maxSpeed  = Common::PLAYER_MAX_SPEED;  ///< Horizontal speed cap.
    bool isGrounded = false;                   ///< Whether entity is resting on a solid surface.
    bool hasJumped  = false;                   ///< Whether the jump action was consumed this airtime.
    float jumpDebounceTimer = 0.0f;            ///< Prevents re-jump immediately after landing (counts down).
    float coyoteTimer = 0.0f;                  ///< Grace period after leaving a ledge (counts down).
};

/// Horizontal direction for level exits.
/// East = player enters from the left, West = player enters from the right.
enum class CardinalDirection { East, West };

/// Collision hitbox with type flags.
struct Collider {
    float width    = Common::DEFAULT_COLLIDER_WIDTH;   ///< Hitbox width (may differ from Transform width).
    float height   = Common::DEFAULT_COLLIDER_HEIGHT;  ///< Hitbox height.
    float offsetX  = 0.0f;    ///< Hitbox X offset relative to Transform.x.
    float offsetY  = 0.0f;    ///< Hitbox Y offset relative to Transform.y.
    bool isTrigger = false;   ///< Overlap-only (no physics resolution); used for exits and hazards.
    bool isSolid   = false;   ///< Solid obstacle; player cannot pass through.
    bool isHazard  = false;   ///< Kills the player on overlap.
};

/// Defines a level transition exit on an entity.
struct LevelExit {
    CardinalDirection direction = CardinalDirection::East;  ///< Which side the player enters from.
    std::string nextLevelPath;    ///< Path to the next .level file.
    float transitionDuration = Common::DEFAULT_TRANSITION_DURATION;  ///< Fade-to-black duration.
};

/// Player-specific movement control settings.
struct PlayerControl {
    float speed   = Common::PLAYER_SPEED;  ///< Horizontal speed scalar (sets Physics::maxSpeed on load).
};

/// Parallax scroll factor for background layers.
struct Parallax {
    float factor  = 1.0f;  ///< 0 = static (follows camera exactly), 1 = full parallax, values in between.
};

} // namespace Gameplay
