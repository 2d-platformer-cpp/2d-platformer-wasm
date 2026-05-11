/// @file WindowManager.hpp
/// RAII wrapper around SDL_Window: creation, fullscreen toggle, and title
/// management.

#pragma once

#include "Common/SDLDeleters.hpp"
#include "Common/Types.hpp"
#include <string>

namespace Engine {

/// RAII wrapper around SDL_Window.
/// Creates the window on construction, handles fullscreen toggle, manages
/// title.
class WindowManager {
public:
  /// Creates an SDL_Window with the given title and size.
  /// @param title   Window title bar text.
  /// @param width   Window client area width in pixels.
  /// @param height  Window client area height in pixels.
  WindowManager(const std::string &title, int width, int height);
  /// Destructor: destroys the SDL_Window via RAII deleter.
  ~WindowManager();

  WindowManager(const WindowManager &) = delete;
  WindowManager &operator=(const WindowManager &) = delete;

  /// Returns the raw SDL_Window pointer (for SDL API calls).
  SDL_Window *get() const { return m_window.get(); }

  /// Updates the window title string (e.g. showing FPS).
  /// @param title  New title text.
  void setTitle(const std::string &title);

  /// Checks for F11 key press in InputState and toggles fullscreen mode.
  /// @param input  Current frame's input snapshot.
  void update(const Common::InputState &input);

private:
  UniqueSDLWindow m_window;      ///< Owned SDL_Window handle.
  bool m_sdlInitialized = false; ///< Whether SDL_Init succeeded.
};

} // namespace Engine
