/// @file MenuSystem.cpp
/// Implementation of the MenuSystem class: main menu and pause menu button
/// layout, click detection with rising-edge debounce, hover state management
/// with configurable debounce timer, and render-command generation.

#include "Gameplay/UI/MenuSystem.hpp"
#include "Common/Constants.hpp"
#include "Common/Colors.hpp"

namespace Gameplay { namespace UI {

bool MenuSystem::isButtonClicked(int mx, int my, float bx, float by, float bw, float bh) {
    /// Point-in-rect test for mouse click detection.
    /// @param mx, my  Mouse coordinates.
    /// @param bx, by  Button top-left.
    /// @param bw, bh  Button dimensions.
    /// @return true if (mx, my) is inside the button rect.
    return mx >= bx && mx <= bx + bw && my >= by && my <= by + bh;
}

void MenuSystem::updateHover(const Common::InputState &input, float deltaTime) {
    /// Updates hover state with a 0.1s debounce to prevent flickering when
    /// the mouse moves between buttons. Resets the timer when the hovered
    /// button changes.
    /// @param input      Current frame input snapshot.
    /// @param deltaTime  Time step in seconds.
    int newHover = -1;
    for (const auto &rect : m_buttonRects) {
        if (input.mouseX >= rect.x && input.mouseX <= rect.x + rect.w
            && input.mouseY >= rect.y && input.mouseY <= rect.y + rect.h) {
            newHover = rect.id;
            break;
        }
    }

    if (newHover != m_lastHoveredButton) {
        m_hoverTimer = 0.0f;
        m_lastHoveredButton = newHover;
    }

    if (newHover >= 0) {
        m_hoverTimer += deltaTime;
        if (m_hoverTimer >= 0.1f)
            m_hoveredButton = newHover;
        else
            m_hoveredButton = -1;
    } else {
        m_hoveredButton = -1;
    }
}

int MenuSystem::updateMainMenu(const Common::InputState &input, std::vector<Common::RenderCommand> &commands,
                                std::vector<Common::TextCommand> &textCommands) {
    /// Builds the three-button main menu (Play, Stats, Exit) with border
    /// outlines and hover highlighting. Detects mouse clicks with rising-edge
    /// debounce and returns the clicked button ID.
    /// @param input         Current frame input snapshot.
    /// @param commands      Render command list to extend.
    /// @param textCommands  Text command list to extend.
    /// @return The ID of the clicked button (1=Play, 2=Stats, 3=Exit), or 0.
    m_buttonRects.clear();

    float cx = (Common::SCREEN_WIDTH - Common::MENU_BUTTON_WIDTH) / 2.0f;
    float startY = (Common::SCREEN_HEIGHT - Common::MENU_BUTTON_HEIGHT * 3 - 40) / 2.0f;

    struct BtnInfo { float y; int id; SDL_Color fill; SDL_Color hover; SDL_Color border; const char *label; };
    BtnInfo btns[] = {
        {startY, 1, {50,205,50,255}, {63,227,63,255}, {40,180,40,255}, "Play"},
        {startY + Common::MENU_BUTTON_HEIGHT + 20, 2, {45,61,58,255}, {60,80,75,255}, {50,205,50,255}, "Stats"},
        {startY + (Common::MENU_BUTTON_HEIGHT + 20) * 2, 3, Common::Colors::BTN_EXIT, Common::Colors::BTN_EXIT_HOVER, {220,120,0,255}, "Exit"},
    };

    float borderSize = 2.0f;

    float btnCenterX = cx + Common::MENU_BUTTON_WIDTH / 2.0f;

    for (auto &btn : btns) {
        bool hovered = (btn.id == m_hoveredButton);
        SDL_Color fill = hovered ? btn.hover : btn.fill;

        m_buttonRects.push_back({cx, btn.y, Common::MENU_BUTTON_WIDTH, Common::MENU_BUTTON_HEIGHT, btn.id});

        commands.push_back({cx - borderSize, btn.y - borderSize,
                            Common::MENU_BUTTON_WIDTH + borderSize * 2,
                            Common::MENU_BUTTON_HEIGHT + borderSize * 2,
                            Common::TextureID::TEX_NONE, 0.0f,
                            btn.border.r, btn.border.g, btn.border.b, btn.border.a});
        commands.push_back({cx, btn.y, Common::MENU_BUTTON_WIDTH, Common::MENU_BUTTON_HEIGHT,
                            Common::TextureID::TEX_NONE, 0.0f,
                            fill.r, fill.g, fill.b, fill.a});

        textCommands.push_back({btn.label, btnCenterX, btn.y + 32, 255, 255, 255, 255, true});
    }

    bool justClicked = input.mouseLeftDown && !m_prevMouseDown;
    m_prevMouseDown = input.mouseLeftDown;

    if (justClicked) {
        for (const auto &rect : m_buttonRects) {
            if (isButtonClicked(input.mouseX, input.mouseY, rect.x, rect.y, rect.w, rect.h))
                return rect.id;
        }
    }
    return 0;
}

int MenuSystem::updatePauseMenu(const Common::InputState &input, std::vector<Common::RenderCommand> &commands,
                                  std::vector<Common::TextCommand> &textCommands) {
    /// Builds the pause menu: full-screen dark overlay, centred panel with
    /// three buttons (Resume, Main Menu, Exit), border outlines, hover
    /// highlighting, and click detection with rising-edge debounce.
    /// @param input         Current frame input snapshot.
    /// @param commands      Render command list to extend.
    /// @param textCommands  Text command list to extend.
    /// @return The ID of the clicked button (1=Resume, 2=Main Menu, 3=Exit), or 0.
    m_buttonRects.clear();

    commands.push_back({0, 0, (float)Common::SCREEN_WIDTH, (float)Common::SCREEN_HEIGHT,
                        Common::TextureID::TEX_NONE, 0.0f,
                        Common::Colors::OVERLAY.r, Common::Colors::OVERLAY.g,
                        Common::Colors::OVERLAY.b, Common::Colors::OVERLAY.a});

    float px = (Common::SCREEN_WIDTH - Common::PAUSE_PANEL_WIDTH) / 2.0f;
    float py = (Common::SCREEN_HEIGHT - Common::PAUSE_PANEL_HEIGHT) / 2.0f;

    commands.push_back({px, py, Common::PAUSE_PANEL_WIDTH, Common::PAUSE_PANEL_HEIGHT,
                        Common::TextureID::TEX_NONE, 0.0f,
                        Common::Colors::PANEL_BG.r, Common::Colors::PANEL_BG.g,
                        Common::Colors::PANEL_BG.b, Common::Colors::PANEL_BG.a});

    float bx = px + (Common::PAUSE_PANEL_WIDTH - Common::MENU_BUTTON_WIDTH) / 2.0f;
    float btnCenterX = bx + Common::MENU_BUTTON_WIDTH / 2.0f;
    float by = py + 10.0f + 3 * 26.0f + 20.0f;

    struct BtnInfo { float y; int id; SDL_Color fill; SDL_Color hover; SDL_Color border; const char *label; };
    BtnInfo btns[] = {
        {by, 1, {50,205,50,255}, {63,227,63,255}, {40,180,40,255}, "Resume"},
        {by + Common::MENU_BUTTON_HEIGHT + 15, 2, {45,61,58,255}, {60,80,75,255}, {50,205,50,255}, "Main Menu"},
        {by + (Common::MENU_BUTTON_HEIGHT + 15) * 2, 3, Common::Colors::BTN_EXIT, Common::Colors::BTN_EXIT_HOVER, {220,120,0,255}, "Exit"},
    };

    float borderSize = 2.0f;

    for (auto &btn : btns) {
        bool hovered = (btn.id == m_hoveredButton);
        SDL_Color fill = hovered ? btn.hover : btn.fill;

        m_buttonRects.push_back({bx, btn.y, Common::MENU_BUTTON_WIDTH, Common::MENU_BUTTON_HEIGHT, btn.id});

        commands.push_back({bx - borderSize, btn.y - borderSize,
                            Common::MENU_BUTTON_WIDTH + borderSize * 2,
                            Common::MENU_BUTTON_HEIGHT + borderSize * 2,
                            Common::TextureID::TEX_NONE, 0.0f,
                            btn.border.r, btn.border.g, btn.border.b, btn.border.a});
        commands.push_back({bx, btn.y, Common::MENU_BUTTON_WIDTH, Common::MENU_BUTTON_HEIGHT,
                            Common::TextureID::TEX_NONE, 0.0f,
                            fill.r, fill.g, fill.b, fill.a});

        textCommands.push_back({btn.label, btnCenterX, btn.y + 32, 255, 255, 255, 255, true});
    }

    bool justClicked = input.mouseLeftDown && !m_prevMouseDown;
    m_prevMouseDown = input.mouseLeftDown;

    if (justClicked) {
        for (const auto &rect : m_buttonRects) {
            if (isButtonClicked(input.mouseX, input.mouseY, rect.x, rect.y, rect.w, rect.h))
                return rect.id;
        }
    }
    return 0;
}

}} // namespace Gameplay::UI
