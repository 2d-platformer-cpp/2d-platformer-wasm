/// @file Camera.cpp
/// Implementation of the Camera class: horizontal player-follow and clamping.

#include "Engine/Camera.hpp"
#include <algorithm>

namespace Engine {

void Camera::update(float playerX) {
    /// Centers the camera on the player, clamping to level bounds.
    /// @param playerX  The player entity's current world X.
    float target = playerX - Common::VIEWPORT_WIDTH / 2.0f;
    m_offsetX = std::clamp(target, m_minX, m_maxX);
}

void Camera::setOffsetX(float value) {
    /// Directly sets the scroll offset, clamped to scroll bounds.
    /// @param value  New offset in logical pixels.
    m_offsetX = std::clamp(value, m_minX, m_maxX);
}

void Camera::setBounds(float minX, float maxX) {
    /// Sets the scroll limits, swapping if out of order.
    /// @param minX  Leftmost valid offset.
    /// @param maxX  Rightmost valid offset.
    if (minX > maxX) std::swap(minX, maxX);
    m_minX = minX;
    m_maxX = maxX;
}

} // namespace Engine
