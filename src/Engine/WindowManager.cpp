/// @file WindowManager.cpp
/// Implementation of the WindowManager class: SDL_Window creation, title,
/// fullscreen toggle, and SDL lifecycle management.

#include "Engine/WindowManager.hpp"
#include <SDL3/SDL.h>

namespace Engine {

WindowManager::WindowManager(const std::string &title, int width, int height) {
    /// Constructs an SDL_Window with the given title and size.
    /// Initialises SDL_VIDEO and SDL_Audio, creates a resizable window,
    /// and sets the minimum window size.
    /// @param title   Window title bar text.
    /// @param width   Client area width in pixels.
    /// @param height  Client area height in pixels.
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) return;
    m_sdlInitialized = true;

    SDL_Window *w = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
    if (!w) return;
    m_window.reset(w);

    SDL_SetWindowMinimumSize(m_window.get(), Common::MINIMUM_SCREEN_WIDTH, Common::MINIMUM_SCREEN_HEIGHT);
}

void WindowManager::setTitle(const std::string &title) {
    /// Updates the window title string (e.g. showing FPS).
    /// @param title  New title text.
    if (m_window)
        SDL_SetWindowTitle(m_window.get(), title.c_str());
}

WindowManager::~WindowManager() {
    /// Destructor: destroys the SDL_Window via RAII deleter and calls SDL_Quit.
    m_window.reset();
    if (m_sdlInitialized) SDL_Quit();
}

void WindowManager::update(const Common::InputState &input) {
    /// Checks for F11 key press (via InputState::toggleFullScreen)
    /// and toggles the SDL_WINDOW_FULLSCREEN flag.
    /// @param input  Current frame's input snapshot.
    if (!input.toggleFullScreen || !m_window) return;
    Uint32 flags = SDL_GetWindowFlags(m_window.get());
    if (flags & SDL_WINDOW_FULLSCREEN)
        SDL_SetWindowFullscreen(m_window.get(), false);
    else
        SDL_SetWindowFullscreen(m_window.get(), true);
}

} // namespace Engine
