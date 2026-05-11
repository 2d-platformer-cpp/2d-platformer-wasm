/// @file Entity.cpp
/// Implementation of Player, StaticObject, and BackgroundLayer entity
/// behaviour: construction, input handling (Player), physics integration,
/// and render-command emission.

#include "Gameplay/Entity.hpp"
#include "Common/Constants.hpp"
#include <algorithm>
#include <cmath>

namespace Gameplay {

Player::Player() {
    /// Constructs a Player with default PlayerControl and Physics components.
    /// The constructor is called by LevelLoader when parsing a "Player" entity
    /// line, or by the LevelEditor when the user places a player spawn point.
    playerControl = PlayerControl{};
    physics = Physics{};
}

void Player::handleInput(const Common::InputState &input, float deltaTime) {
    /// Reads horizontal input (left/right) and applies acceleration or
    /// friction to the player's velocity. Handles jump activation:
    /// triggers on Space/W when grounded, during coyote time, or after a
    /// jump-debounce period. Caps horizontal velocity at maxSpeed.
    /// @param input      Current frame input snapshot.
    /// @param deltaTime  Time step in seconds.
    if (!playerControl || !physics) return;

    float accelX = 0.0f;
    if (input.left)  accelX -= Common::PLAYER_ACCELERATION;
    if (input.right) accelX += Common::PLAYER_ACCELERATION;

    if (accelX == 0.0f) {
        if (physics->velocityX > 0)
            physics->velocityX = std::max(0.0f, physics->velocityX - physics->friction * deltaTime);
        else if (physics->velocityX < 0)
            physics->velocityX = std::min(0.0f, physics->velocityX + physics->friction * deltaTime);
    } else {
        physics->velocityX += accelX * deltaTime;
    }

    physics->velocityX = std::clamp(physics->velocityX, -physics->maxSpeed, physics->maxSpeed);

    if ((input.jump || input.up) && (physics->isGrounded || physics->coyoteTimer > 0.0f) && !physics->hasJumped && physics->jumpDebounceTimer <= 0.0f) {
        physics->velocityY = Common::JUMP_FORCE;
        physics->isGrounded = false;
        physics->hasJumped = true;
        physics->coyoteTimer = 0.0f;
    }
}

void Player::update(float deltaTime) {
    /// Integrates gravity and velocity into position each fixed timestep.
    /// Decrements jump debounce and coyote timers. Caps vertical velocity
    /// at TERMINAL_VELOCITY.
    /// @param deltaTime  Fixed time step in seconds.
    if (!physics) return;
    if (physics->jumpDebounceTimer > 0.0f)
        physics->jumpDebounceTimer = std::max(0.0f, physics->jumpDebounceTimer - deltaTime);
    if (!physics->isGrounded && physics->coyoteTimer > 0.0f)
        physics->coyoteTimer -= deltaTime;
    physics->velocityY += physics->gravity * deltaTime;
    physics->velocityY = std::min(physics->velocityY, Common::TERMINAL_VELOCITY);
    transform.x += physics->velocityX * deltaTime;
    transform.y += physics->velocityY * deltaTime;
}

void Player::render(std::vector<Common::RenderCommand> &commands) {
    /// Pushes a RenderCommand using the entity's sprite textureID (or
    /// TEX_PLAYER fallback) at the player's transform position.
    /// @param commands  Render command list to extend.
    if (isDestroyed) return;
    commands.push_back({
        transform.x, transform.y,
        transform.width, transform.height,
        sprite ? sprite->textureID : Common::TextureID::TEX_PLAYER,
        1.0f, 0, 255, 255, 255
    });
}

void StaticObject::render(std::vector<Common::RenderCommand> &commands) {
    /// Pushes a RenderCommand using the entity's sprite textureID (or
    /// TEX_PLATFORM fallback). Fallback colour is green (0,255,0,255)
    /// which the Renderer uses when the texture is absent.
    /// @param commands  Render command list to extend.
    if (isDestroyed) return;
    commands.push_back({
        transform.x, transform.y,
        transform.width, transform.height,
        sprite ? sprite->textureID : Common::TextureID::TEX_PLATFORM,
        1.0f, 0, 255, 0, 255
    });
}

void BackgroundLayer::render(std::vector<Common::RenderCommand> &commands) {
    /// Pushes a RenderCommand using the entity's sprite textureID (or
    /// TEX_BACKGROUND_FAR fallback) and parallax scroll factor. White
    /// fallback colour at full opacity.
    /// @param commands  Render command list to extend.
    if (isDestroyed) return;
    commands.push_back({
        transform.x, transform.y,
        transform.width, transform.height,
        sprite ? sprite->textureID : Common::TextureID::TEX_BACKGROUND_FAR,
        parallax ? parallax->factor : 1.0f, 255, 255, 255, 255
    });
}

} // namespace Gameplay
