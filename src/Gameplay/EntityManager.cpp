/// @file EntityManager.cpp
/// Implementation of the EntityManager class: entity lifecycle (add, clear,
/// update, remove), z-sorted rendering, player lookup, and AABB collision
/// detection with overlap-magnitude resolution.

#include "Gameplay/EntityManager.hpp"
#include "Common/Constants.hpp"
#include <array>
#include <algorithm>
#include <numeric>

namespace Gameplay {

void EntityManager::addEntity(std::shared_ptr<Entity> entity) {
    /// Assigns a unique incrementing ID to the entity and adds it to the
    /// internal entity vector.
    /// @param entity  Shared pointer to the entity.
    entity->id = m_nextEntityID++;
    m_entities.push_back(std::move(entity));
}

void EntityManager::clear() {
    /// Removes all entities immediately.
    m_entities.clear();
}

void EntityManager::update(float deltaTime, const Common::InputState &input) {
    /// Advances all living entities by one fixed timestep: delegates input
    /// to Player entities, calls entity->update(), applies wall-clamping
    /// and bounds-floor checks, detects fall-death (Y > SCREEN_HEIGHT),
    /// and garbage-collects destroyed entities at the end.
    /// @param deltaTime  Fixed time step in seconds.
    /// @param input      Current frame input snapshot.
    for (size_t i = 0; i < m_entities.size();) {
        auto &entity = m_entities[i];
        if (entity->isDestroyed) { ++i; continue; }

        if (auto player = std::dynamic_pointer_cast<Player>(entity))
            player->handleInput(input, deltaTime);

        entity->update(deltaTime);

        if (entity->isDestroyed) { ++i; continue; }

        if (entity->physics) {
            if (auto player = std::dynamic_pointer_cast<Player>(entity)) {
                if (entity->transform.y > Common::SCREEN_HEIGHT) {
                    m_deathByFall = true;
                }
            }
            if (entity->transform.y > Common::SCREEN_HEIGHT * 2) {
                entity->transform.y = Common::SCREEN_HEIGHT * 2;
                entity->physics->velocityY = 0;
                if (auto player = std::dynamic_pointer_cast<Player>(entity))
                    entity->physics->isGrounded = true;
            }
            if (levelConfig.isLeftWallClamped && entity->transform.x < 0) {
                entity->transform.x = 0;
                entity->physics->velocityX = 0;
            }
            if (levelConfig.isRightWallClamped &&
                entity->transform.x > levelConfig.levelWidth - entity->transform.width) {
                entity->transform.x = levelConfig.levelWidth - entity->transform.width;
                entity->physics->velocityX = 0;
            }
        }

        ++i;
    }

    m_entities.erase(std::remove_if(m_entities.begin(), m_entities.end(),
        [](const auto &e) { return e->isDestroyed; }), m_entities.end());
}

void EntityManager::render(std::vector<Common::RenderCommand> &commands) {
    /// Sorts entity indices by zIndex (ascending, so lower =
    /// drawn first / behind) and calls each entity's render() method.
    /// @param commands  Render command list to extend.
    std::vector<size_t> indices(m_entities.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(),
        [&](size_t a, size_t b) { return m_entities[a]->transform.zIndex < m_entities[b]->transform.zIndex; });

    for (size_t i : indices)
        if (!m_entities[i]->isDestroyed) m_entities[i]->render(commands);
}

std::shared_ptr<Player> EntityManager::getPlayer() const {
    /// Scans the entity list for the first living Player entity.
    /// @return Shared pointer to the Player, or nullptr if none exists.
    for (const auto &entity : m_entities)
        if (auto player = std::dynamic_pointer_cast<Player>(entity))
            return player;
    return nullptr;
}

std::optional<LevelExit> EntityManager::checkCollisions() {
    /// Performs AABB overlap detection between the player and all other
    /// entities with colliders. Resolves solid overlaps via pure overlap-
    /// magnitude comparison (no velocity overrides except zeroing the
    /// resolved axis). Sets isGrounded when the player lands on top of a
    /// solid, and resets hasJumped + jumpDebounceTimer on landing.
    /// Returns LevelExit info if the player overlaps an exit trigger,
    /// sets m_deathByHazard if overlapping a hazard, or returns nullopt.
    /// @return LevelExit info if the player triggered an exit, nullopt otherwise.
    auto player = getPlayer();
    if (!player || !player->collider) return std::nullopt;

    bool prevGrounded = player->physics ? player->physics->isGrounded : false;
    if (player->physics) player->physics->isGrounded = false;

    auto playerRect = [&]() -> std::array<float, 4> {
        return {
            player->transform.x + player->collider->offsetX,
            player->transform.x + player->collider->offsetX + player->collider->width,
            player->transform.y + player->collider->offsetY,
            player->transform.y + player->collider->offsetY + player->collider->height
        };
    }();

    for (const auto &entity : m_entities) {
        if (entity == player || entity->isDestroyed || !entity->collider) continue;

        auto otherRect = [&]() -> std::array<float, 4> {
            return {
                entity->transform.x + entity->collider->offsetX,
                entity->transform.x + entity->collider->offsetX + entity->collider->width,
                entity->transform.y + entity->collider->offsetY,
                entity->transform.y + entity->collider->offsetY + entity->collider->height
            };
        }();

        bool overlaps = (playerRect[0] < otherRect[1] && playerRect[1] > otherRect[0] &&
                         playerRect[2] < otherRect[3] && playerRect[3] > otherRect[2]);
        if (!overlaps) continue;

        if (entity->collider->isHazard) {
            m_deathByHazard = true;
            continue;
        }

        if (entity->exitEast && entity->exitWest) {
            float playerCenterX = player->transform.x + player->collider->offsetX + player->collider->width / 2.0f;
            float entityCenterX = entity->transform.x + entity->collider->offsetX + entity->collider->width / 2.0f;
            return playerCenterX < entityCenterX ? *entity->exitEast : *entity->exitWest;
        }
        if (entity->exitEast) return *entity->exitEast;
        if (entity->exitWest) return *entity->exitWest;

        if (!entity->collider->isSolid) continue;

        float overlapLeft   = playerRect[1] - otherRect[0];
        float overlapRight  = otherRect[1] - playerRect[0];
        float overlapTop    = playerRect[3] - otherRect[2];
        float overlapBottom = otherRect[3] - playerRect[2];

        if (overlapLeft < overlapRight && overlapLeft < overlapTop && overlapLeft < overlapBottom) {
            player->transform.x -= overlapLeft;
            playerRect[0] = player->transform.x + player->collider->offsetX;
            playerRect[1] = playerRect[0] + player->collider->width;
            if (player->physics) player->physics->velocityX = 0;
        } else if (overlapRight < overlapTop && overlapRight < overlapBottom) {
            player->transform.x += overlapRight;
            playerRect[0] = player->transform.x + player->collider->offsetX;
            playerRect[1] = playerRect[0] + player->collider->width;
            if (player->physics) player->physics->velocityX = 0;
        } else if (overlapTop < overlapBottom) {
            player->transform.y -= overlapTop;
            playerRect[2] = player->transform.y + player->collider->offsetY;
            playerRect[3] = playerRect[2] + player->collider->height;
            if (player->physics) {
                player->physics->velocityY = 0;
                player->physics->isGrounded = true;
                player->physics->coyoteTimer = Common::COYOTE_TIME;
                if (!prevGrounded) {
                    player->physics->hasJumped = false;
                    player->physics->jumpDebounceTimer = Common::GROUND_DEBOUNCE_TIME;
                }
            }
        } else {
            player->transform.y += overlapBottom;
            playerRect[2] = player->transform.y + player->collider->offsetY;
            playerRect[3] = playerRect[2] + player->collider->height;
            if (player->physics) player->physics->velocityY = 0;
        }
    }
    return std::nullopt;
}

} // namespace Gameplay
