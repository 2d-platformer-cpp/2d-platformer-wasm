/// @file Colors.hpp
/// Named SDL_Color constants used throughout the game for text, buttons,
/// panels, and overlays.

#pragma once

#include <SDL3/SDL.h>

namespace Common { namespace Colors {

    /// Primary text: pure white, used for headings and default text.
    inline constexpr SDL_Color TEXT_PRIMARY  = {255, 255, 255, 255};
    /// Accent text: cyan, used for player name and level name in HUD.
    inline constexpr SDL_Color TEXT_ACCENT   = {0,   255, 255, 255};
    /// Death-related text: orange, used for death count in HUD.
    inline constexpr SDL_Color TEXT_DEATHS   = {255, 140, 0,   255};
    /// Dim/muted text: grey, used for secondary info (press ESC hint, best time).
    inline constexpr SDL_Color TEXT_DIM      = {136, 136, 136, 255};

    /// "Play" / "Resume" button: green fill.
    inline constexpr SDL_Color BTN_PLAY        = {50,  205, 50,  255};
    /// "Play" / "Resume" button: brighter green on hover.
    inline constexpr SDL_Color BTN_PLAY_HOVER  = {63,  227, 63,  255};
    /// "Stats" / "Main Menu" button: dark teal fill.
    inline constexpr SDL_Color BTN_STATS       = {45,  61,  58,  255};
    /// "Stats" / "Main Menu" button: lighter teal on hover.
    inline constexpr SDL_Color BTN_STATS_HOVER = {60,  80,  75,  255};
    /// "Exit" button: orange fill.
    inline constexpr SDL_Color BTN_EXIT        = {255, 140, 0,   255};
    /// "Exit" button: brighter orange on hover.
    inline constexpr SDL_Color BTN_EXIT_HOVER  = {255, 165, 30,  255};

    /// Panel background: dark green at 85 % opacity, used for stats/pause panels.
    inline constexpr SDL_Color PANEL_BG     = {26,  37,  32,  217};
    /// Panel border: green, used as outline behind panels.
    inline constexpr SDL_Color PANEL_BORDER = {50,  205, 50,  255};
    /// Full-screen overlay: black at ~70 %, used behind pause menu and stats screen.
    inline constexpr SDL_Color OVERLAY      = {0,   0,   0,   180};

}} // namespace Common::Colors
