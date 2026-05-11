/// @file Types.hpp
/// Common type definitions for the game engine: game state, input, render/text commands.

#pragma once

#include "Common/Constants.hpp"
#include <string>

namespace Common
{
    /// Top-level state machine for the game loop.
    /// Determines which input handlers, update logic, and render paths are active.
    enum class GameState
    {
        WELCOME,       ///< Initial splash screen, auto-transitions to MAIN_MENU after 2s
        MAIN_MENU,     ///< Title screen with Play / Stats / Exit buttons
        STATS_SCREEN,  ///< Player statistics overlay accessed from MAIN_MENU
        RUNNING,       ///< Active gameplay or level editor
        PAUSED,        ///< Gameplay frozen, pause menu overlay shown
        LEVEL_COMPLETE,///< Post-level completion overlay, waiting for user input
        EXITING        ///< Signals the game loop to terminate
    };

    /// Snapshot of all keyboard and mouse state for a single frame.
    /// Polled once per fixed timestep by InputManager, consumed by Game and LevelEditor.
    struct InputState
    {
        // --- Movement & actions ---
        bool up = false;           ///< W key – jump (also used by editor for move-up)
        bool down = false;         ///< S key – move down (editor only)
        bool left = false;         ///< A key – move left / strafe
        bool right = false;        ///< D key – move right / strafe
        bool jump = false;         ///< Space key – jump (identical to up for gameplay)
        bool attack = false;       ///< F key – attack action (reserved)
        bool toggleFullScreen = false; ///< F11 key – toggle fullscreen
        bool quit = false;         ///< Window close event – terminate app

        // --- Editor camera movement ---
        bool arrowLeft = false;    ///< Left arrow – scroll camera left in editor
        bool arrowRight = false;   ///< Right arrow – scroll camera right in editor

        // --- Editor hotkeys (single-letter) ---
        bool c = false;            ///< C key – create new StaticObject
        bool v = false;            ///< V key – create Player spawn point
        bool b = false;            ///< B key – create BackgroundLayer
        bool t = false;            ///< T key – cycle texture of selected entity
        bool y = false;            ///< Y key – cycle collider type (solid/trigger/hazard/none)
        bool u = false;            ///< U key – toggle exit properties
        bool r = false;            ///< R key – cycle Z-index
        bool l = false;            ///< L key – toggle left wall clamp (Shift+L for right)

        // --- Text / confirmation input ---
        bool backspace = false;    ///< Backspace key – character deletion
        bool enter = false;        ///< Enter key – submit / confirm
        bool shift = false;        ///< Shift key – modifier for resize mode in editor

        // --- Game state toggles ---
        bool pause = false;        ///< ESC key – toggle pause / return to menu

        // --- Editor level resizing ---
        bool leftBracket = false;  ///< [ key – decrease level width by 256px
        bool rightBracket = false; ///< ] key – increase level width by 256px

        // --- Editor texture presets (number keys) ---
        bool num1 = false;         ///< 1 – set texture to TEX_PLATFORM
        bool num2 = false;         ///< 2 – set texture to TEX_FLOOR
        bool num3 = false;         ///< 3 – set texture to TEX_HAZARD_LAVA
        bool num4 = false;         ///< 4 – set texture to TEX_HAZARD_SPIKE
        bool num5 = false;         ///< 5 – set texture to TEX_BACKGROUND_FAR
        bool num6 = false;         ///< 6 – set texture to TEX_BACKGROUND_MID
        bool num7 = false;         ///< 7 – set texture to TEX_BACKGROUND_NEAR
        bool num8 = false;         ///< 8 – set texture to TEX_PLAYER
        bool num9 = false;         ///< 9 – set texture to TEX_ENEMY

        // --- Mouse state ---
        int mouseX = 0;            ///< Mouse X in logical pixels (after window→logical conversion)
        int mouseY = 0;            ///< Mouse Y in logical pixels
        bool mouseLeftDown = false;///< Left mouse button currently held
    };

    /// A single draw command issued by entities, UI, or backgrounds.
    /// The Renderer iterates these each frame: textured draw or colored-rect fallback.
    struct RenderCommand
    {
        float x = 0.0f, y = 0.0f;                                          ///< Destination top-left in world/logical coordinates
        float width = 0.0f, height = 0.0f;                                 ///< Destination size
        Common::TextureID textureID = Common::TextureID::TEX_NONE;          ///< Texture to draw, or TEX_NONE for solid color
        float scrollFactor = 1.0f;                                          ///< Parallax multiplier (0=fully static, 1=follows camera)
        unsigned char colorR = 0, colorG = 255, colorB = 255, colorA = 255;///< Fallback color used when textureID is TEX_NONE or missing
        int srcX = 0, srcY = 0, srcW = 0, srcH = 0;                        ///< Source rect within the texture (0=use full texture)
    };

    /// A text rendering command issued by menus, HUD, or editor overlay.
    /// The Renderer converts these to SDL_Textures and draws them.
    struct TextCommand
    {
        std::string text;                        ///< The string to render
        float x = 0.0f, y = 0.0f;               ///< Position (top-left, centered, or right-aligned as per flags)
        unsigned char colorR = 255, colorG = 255, colorB = 255, colorA = 255; ///< Text color (RGBA)
        bool centered = false;                   ///< If true, x is treated as center
        std::string font = "ui";                 ///< Font name registered with TextRenderer
        bool rightAligned = false;               ///< If true, x is treated as right-edge offset from screen right
    };

} // namespace Common
