/// @file ProgressIndicator.cpp
/// Implementation of the ProgressIndicator class: top-right minimap-style
/// progress bar showing how far the player is through the level.

#include "Gameplay/UI/ProgressIndicator.hpp"
#include <algorithm>

namespace Gameplay { namespace UI {

ProgressIndicator::ProgressIndicator(float x, float y, float width, float height)
    /// Constructs the progress bar at the given screen position and size.
    /// @param x, y    Position in logical pixels (top-right of screen).
    /// @param width   Total bar width in pixels.
    /// @param height  Bar height in pixels.
    : m_x(x), m_y(y), m_width(width), m_height(height) {}

void ProgressIndicator::update(float playerX, float worldWidth) {
    /// Calculates the fill width as a ratio of playerX / worldWidth,
    /// clamped to [0, 1].
    /// @param playerX     Current player world X.
    /// @param worldWidth  Total level width.
    float ratio = std::clamp(playerX / worldWidth, 0.0f, 1.0f);
    m_fillWidth = ratio * m_width;
}

void ProgressIndicator::render(std::vector<Common::RenderCommand> &commands) const {
    /// Pushes two RenderCommands: a dark-grey background track and a green
    /// fill rect representing the current progress.
    /// @param commands  Render command list to extend.
    commands.push_back({m_x, m_y, m_width, m_height,
                        Common::TextureID::TEX_NONE, 0.0f, 80, 80, 80, 200});
    commands.push_back({m_x, m_y, m_fillWidth, m_height,
                        Common::TextureID::TEX_NONE, 0.0f, 0, 200, 0, 255});
}

}} // namespace Gameplay::UI
