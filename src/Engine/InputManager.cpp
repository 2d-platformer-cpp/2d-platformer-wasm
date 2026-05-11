/// @file InputManager.cpp
/// Implementation of the InputManager class: SDL event polling, keyboard
/// state snapshot, and mouse coordinate conversion.

#include "Engine/InputManager.hpp"
#include <SDL3/SDL.h>

namespace Engine {

Common::InputState InputManager::update() {
    /// Internal helper: polls SDL events and reads keyboard state without
    /// mouse coordinate conversion. Resets toggleFullScreen each frame
    /// (reacts on rising edge only).
    /// @return Fully populated InputState for the current frame.
    m_state.toggleFullScreen = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) m_state.quit = true;
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F11)
            m_state.toggleFullScreen = true;
    }

    const bool *keys = SDL_GetKeyboardState(nullptr);
    m_state.left        = keys[SDL_SCANCODE_A];
    m_state.right       = keys[SDL_SCANCODE_D];
    m_state.up          = keys[SDL_SCANCODE_W];
    m_state.down        = keys[SDL_SCANCODE_S];
    m_state.jump        = keys[SDL_SCANCODE_SPACE];
    m_state.attack      = keys[SDL_SCANCODE_F];
    m_state.arrowLeft   = keys[SDL_SCANCODE_LEFT];
    m_state.arrowRight  = keys[SDL_SCANCODE_RIGHT];
    m_state.c           = keys[SDL_SCANCODE_C];
    m_state.v           = keys[SDL_SCANCODE_V];
    m_state.b           = keys[SDL_SCANCODE_B];
    m_state.t           = keys[SDL_SCANCODE_T];
    m_state.y           = keys[SDL_SCANCODE_Y];
    m_state.u           = keys[SDL_SCANCODE_U];
    m_state.r           = keys[SDL_SCANCODE_R];
    m_state.l           = keys[SDL_SCANCODE_L];
    m_state.backspace   = keys[SDL_SCANCODE_BACKSPACE];
    m_state.enter       = keys[SDL_SCANCODE_RETURN];
    m_state.shift       = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    m_state.pause       = keys[SDL_SCANCODE_ESCAPE];
    m_state.leftBracket  = keys[SDL_SCANCODE_LEFTBRACKET];
    m_state.rightBracket = keys[SDL_SCANCODE_RIGHTBRACKET];

    m_state.num1 = keys[SDL_SCANCODE_1];
    m_state.num2 = keys[SDL_SCANCODE_2];
    m_state.num3 = keys[SDL_SCANCODE_3];
    m_state.num4 = keys[SDL_SCANCODE_4];
    m_state.num5 = keys[SDL_SCANCODE_5];
    m_state.num6 = keys[SDL_SCANCODE_6];
    m_state.num7 = keys[SDL_SCANCODE_7];
    m_state.num8 = keys[SDL_SCANCODE_8];
    m_state.num9 = keys[SDL_SCANCODE_9];

    float mx, my;
    SDL_GetMouseState(&mx, &my);
    m_state.mouseX = static_cast<int>(mx);
    m_state.mouseY = static_cast<int>(my);
    m_state.mouseLeftDown = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT);

    return m_state;
}

Common::InputState InputManager::update(SDL_Renderer *renderer) {
    /// Polls SDL events, reads keyboard state, and converts mouse
    /// coordinates from window space to logical presentation space.
    /// @param renderer  SDL_Renderer used for logical coordinate conversion.
    /// @return Fully populated InputState for the current frame.
    auto state = update();
    float mx = static_cast<float>(state.mouseX);
    float my = static_cast<float>(state.mouseY);
    SDL_RenderCoordinatesFromWindow(renderer, mx, my, &mx, &my);
    state.mouseX = static_cast<int>(mx);
    state.mouseY = static_cast<int>(my);
    return state;
}

} // namespace Engine
