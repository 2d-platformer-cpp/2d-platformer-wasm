/// @file Renderer.hpp
/// Manages the SDL_Renderer, texture cache, and all draw operations.
/// Entity rendering, parallax backgrounds, UI panels, and overlays all go through this class.

#pragma once

#include <SDL3/SDL.h>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include "Common/Types.hpp"
#include "Common/SDLDeleters.hpp"

namespace Engine {

/// Manages the SDL_Renderer, texture cache, and all draw operations.
/// Entity rendering, parallax backgrounds, UI panels, overlays, and text textures
/// all go through this class.
class Renderer {
public:
    /// Creates an SDL_Renderer from the given window.
    /// Enables VSync and sets logical presentation to SCREEN_WIDTH x SCREEN_HEIGHT.
    /// @param window  The SDL_Window to render into.
    explicit Renderer(SDL_Window *window);
    /// Default destructor; SDL_Renderer freed via RAII deleter.
    ~Renderer() = default;

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    /// Returns the raw SDL_Renderer pointer for direct SDL calls.
    SDL_Renderer *get() const { return m_sdlRenderer.get(); }

    /// Loads a PNG texture from disk and associates it with a TextureID.
    /// @param id    The TextureID to store the texture under.
    /// @param path  Filesystem path to the PNG file.
    /// @return true on success, false on any error.
    bool loadTexture(Common::TextureID id, const std::string &path);

    /// Clears the backbuffer to black. Call at the start of each frame.
    void beginFrame();

    /// Presents the rendered frame to the screen. Call at the end of each frame.
    void endFrame();

    /// Processes a batch of RenderCommands.
    /// Background textures (FAR/MID/NEAR) are tiled horizontally with parallax offset.
    /// Other textures are drawn once; missing textures fall back to colored rectangles.
    /// @param commands        The command list to process.
    /// @param cameraOffsetX   Current camera scroll offset (subtracted based on scrollFactor).
    void drawCommands(const std::vector<Common::RenderCommand> &commands, float cameraOffsetX = 0.0f);

    /// Convenience wrapper: draws UI commands with cameraOffsetX = 0 (non-scrolling).
    /// @param commands  UI render commands (buttons, panels, overlays).
    void drawUI(const std::vector<Common::RenderCommand> &commands);

    /// Draws a full-screen semi-transparent black overlay.
    /// @param alpha  Opacity from 0.0 (transparent) to 255.0 (fully opaque).
    void drawOverlay(float alpha);

    /// Directly draws an SDL_Texture at the given position and size.
    /// @param texture  The SDL_Texture to draw (must not be null).
    /// @param x, y     Destination top-left.
    /// @param w, h     Destination dimensions.
    void drawTexture(SDL_Texture *texture, float x, float y, float w, float h);

    /// Looks up a previously loaded texture by its TextureID.
    /// @param id  The texture identifier.
    /// @return Pointer to the SDL_Texture, or nullptr if not loaded.
    SDL_Texture *getTexture(Common::TextureID id) const;

private:
    UniqueSDLRenderer m_sdlRenderer;                           ///< Owned SDL_Renderer handle.
    std::unordered_map<Common::TextureID, UniqueSDLTexture> m_textureCache;  ///< Loaded textures by ID.
};

} // namespace Engine
