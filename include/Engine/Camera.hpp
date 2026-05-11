/// @file Camera.hpp
/// Horizontal scrolling camera that follows the player's X position.
/// Clamped within level bounds so the viewport never shows empty space beyond level edges.

#pragma once

#include "Common/Constants.hpp"

namespace Engine {

/// Horizontal scrolling camera that follows the player's X position.
/// Clamped within level bounds so the viewport never shows empty space beyond level edges.
class Camera {
public:
    /// Recenters the camera on playerX, then clamps to [m_minX, m_maxX].
    /// @param playerX  The player entity's current world X.
    void update(float playerX);

    /// Current horizontal scroll offset in logical pixels.
    float getOffsetX() const { return m_offsetX; }

    /// Directly sets the scroll offset (used by the level editor for arrow-key scrolling).
    /// @param value  New offset, will be clamped to bounds.
    void setOffsetX(float value);

    /// Sets the scroll limits.
    /// @param minX  Leftmost valid offset (usually 0).
    /// @param maxX  Rightmost valid offset (levelWidth - viewportWidth).
    void setBounds(float minX, float maxX);

private:
    float m_offsetX = 0.0f;                 ///< Current horizontal scroll offset.
    float m_minX = Common::DEFAULT_CAMERA_MIN_X;  ///< Minimum scroll boundary.
    float m_maxX = Common::DEFAULT_CAMERA_MAX_X;  ///< Maximum scroll boundary.
};

} // namespace Engine
