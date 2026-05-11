/// @file MenuSystem.hpp
/// Renders and handles interaction for the main menu and pause menu.
/// Manages button hover state with a configurable debounce timer.

#pragma once

#include <vector>
#include "Common/Types.hpp"

namespace Gameplay { namespace UI {

/// Describes a clickable button rect for hover/click hit testing.
struct ButtonRect {
    float x, y;  ///< Top-left corner.
    float w, h;  ///< Width and height.
    int id;       ///< Unique button ID (returned when clicked).
};

/// Renders and handles interaction for the main menu and pause menu.
/// Manages button hover state with a configurable debounce timer.
class MenuSystem {
public:
    /// Default constructor.
    MenuSystem() = default;

    /// Builds and returns the main menu (Play, Stats, Exit).
    /// @param input         Current frame input snapshot.
    /// @param commands      Render command list to extend (button backgrounds + borders).
    /// @param textCommands  Text command list to extend (button labels).
    /// @return The ID of the clicked button, or 0 if none was clicked.
    int updateMainMenu(const Common::InputState &input, std::vector<Common::RenderCommand> &commands,
                        std::vector<Common::TextCommand> &textCommands);

    /// Builds and returns the pause menu (Resume, Main Menu, Exit).
    /// @param input         Current frame input snapshot.
    /// @param commands      Render command list to extend (overlay, panel, buttons).
    /// @param textCommands  Text command list to extend (button labels).
    /// @return The ID of the clicked button, or 0 if none was clicked.
    int updatePauseMenu(const Common::InputState &input, std::vector<Common::RenderCommand> &commands,
                         std::vector<Common::TextCommand> &textCommands);

    /// Point-in-rect test for mouse click detection.
    /// @param mx, my  Mouse coordinates.
    /// @param bx, by  Button top-left.
    /// @param bw, bh  Button dimensions.
    /// @return true if (mx, my) is inside the button rect.
    bool isButtonClicked(int mx, int my, float bx, float by, float bw, float bh);

    /// Updates hover state with a small debounce delay (0.1s) to prevent flicker.
    /// @param input      Current frame input snapshot.
    /// @param deltaTime  Time step in seconds.
    void updateHover(const Common::InputState &input, float deltaTime);

    /// Returns the ID of the currently hovered button, or -1 if none.
    int getHoveredButton() const { return m_hoveredButton; }

private:
    bool m_prevMouseDown = false;            ///< Mouse button previous state for edge detection.
    std::vector<ButtonRect> m_buttonRects;  ///< Current frame's button rects.
    float m_hoverTimer = 0.0f;              ///< Accumulator for hover debounce.
    int m_hoveredButton = -1;               ///< Currently hovered button ID.
    int m_lastHoveredButton = -1;           ///< Previously hovered button ID.
};

}} // namespace Gameplay::UI
