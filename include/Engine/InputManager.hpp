/// @file InputManager.hpp
/// Polls SDL events and keyboard state once per fixed timestep.
/// Produces a Common::InputState snapshot consumed by Game and LevelEditor.

#pragma once

#include <SDL3/SDL.h>
#include "Common/Types.hpp"

namespace Engine {

/// Polls SDL events and keyboard state once per fixed timestep.
/// Produces a Common::InputState snapshot consumed by Game and LevelEditor.
class InputManager {
public:
    /// Polls SDL events, reads keyboard state, converts mouse coordinates from
    /// window space to logical presentation space, and returns the InputState.
    /// @param renderer  SDL_Renderer used for logical coordinate conversion.
    /// @return Fully populated InputState for the current frame.
    Common::InputState update(SDL_Renderer *renderer);

private:
    /// Internal: polls events + keyboard without coordinate conversion.
    /// Called by the public update(renderer) overload.
    Common::InputState update();
    Common::InputState m_state{};  ///< Persistent state between calls (toggles, etc.)
};

} // namespace Engine
