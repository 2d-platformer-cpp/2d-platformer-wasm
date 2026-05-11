/// @file TextRenderer.hpp
/// SDL_ttf wrapper managing font loading, text rasterization, and measurement.
/// Allocates SDL_Textures on demand; caller must keep them alive for the current frame.

#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include "Common/SDLDeleters.hpp"

namespace Engine {

/// SDL_ttf wrapper managing font loading, text rasterization, and measurement.
/// Allocates SDL_Textures on demand; caller must keep them alive for the frame.
class TextRenderer {
public:
    /// Default constructor; call initialize() before use.
    TextRenderer() = default;
    /// Destructor: calls shutdown() to free fonts and TTF_Quit.
    ~TextRenderer();

    TextRenderer(const TextRenderer &) = delete;
    TextRenderer &operator=(const TextRenderer &) = delete;

    /// Initialises the SDL_ttf library. Must be called before any font operations.
    /// @return true on success, false if TTF_Init failed.
    bool initialize();

    /// Frees all loaded fonts and shuts down SDL_ttf.
    void shutdown();

    /// Loads a TrueType font from disk and registers it under a name.
    /// @param name  Key used to reference this font later (e.g. "ui", "title").
    /// @param path  Filesystem path to the .ttf file.
    /// @param size  Font size in points.
    /// @return true if the font was loaded successfully.
    bool loadFont(const std::string &name, const std::string &path, float size);

    /// Switches the active font for subsequent renderText / measureText calls.
    /// @param name  The font name as passed to loadFont.
    void setActiveFont(const std::string &name);

    /// Renders a string to a new SDL_Texture using the active font and color.
    /// The caller owns the returned texture (or UniqueSDLTexture).
    /// @param renderer  SDL_Renderer to create the texture from.
    /// @param text      The string to render.
    /// @param color     Text color (RGBA).
    /// @return A new SDL_Texture, or nullptr on failure.
    UniqueSDLTexture renderText(SDL_Renderer *renderer,
                                 const std::string &text,
                                 SDL_Color color);

    /// Measures the pixel dimensions the given text would occupy with the active font.
    /// @param text  The string to measure.
    /// @param outW  [out] Width in pixels.
    /// @param outH  [out] Height in pixels.
    void measureText(const std::string &text, float &outW, float &outH) const;

private:
    std::unordered_map<std::string, UniqueTTFFont> m_fonts;  ///< Named font registry.
    TTF_Font *m_activeFont = nullptr;                         ///< Currently selected font.
    bool m_ttfInitialized = false;                            ///< Whether TTF_Init succeeded.
};

} // namespace Engine
