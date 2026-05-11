/// @file ProgressIndicator.hpp
/// Top-right minimap-style progress bar showing how far the player is through the level.

#pragma once

#include <vector>
#include "Common/Types.hpp"

namespace Gameplay { namespace UI {

/// Top-right minimap-style progress bar showing how far the player is through the level.
class ProgressIndicator {
public:
    /// @param x, y    Position in logical pixels (top-right of screen).
    /// @param width   Total bar width in pixels.
    /// @param height  Bar height in pixels.
    ProgressIndicator(float x, float y, float width, float height);

    /// Calculates the fill width based on player X position relative to world width.
    /// @param playerX     Current player world X.
    /// @param worldWidth  Total level width.
    void update(float playerX, float worldWidth);

    /// Pushes background and fill rect RenderCommands.
    /// @param commands  Render command list to extend.
    void render(std::vector<Common::RenderCommand> &commands) const;

private:
    float m_x, m_y;            ///< Bar position.
    float m_width, m_height;   ///< Bar dimensions.
    float m_fillWidth = 0.0f;  ///< Current fill width computed by update().
};

}} // namespace Gameplay::UI
