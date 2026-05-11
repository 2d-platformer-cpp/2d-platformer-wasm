/// @file TextRenderer.cpp
/// Implementation of the TextRenderer class: SDL_ttf initialisation,
/// font loading, text rasterisation, and measurement.

#include "Engine/TextRenderer.hpp"
#include <SDL3_ttf/SDL_ttf.h>

namespace Engine {

TextRenderer::~TextRenderer() {
    /// Destructor: calls shutdown() to free all loaded fonts and quit SDL_ttf.
    shutdown();
}

bool TextRenderer::initialize() {
    /// Initialises the SDL_ttf library.
    /// @return true on success, false if TTF_Init failed.
    if (!TTF_Init()) {
        SDL_Log("TextRenderer: TTF_Init failed: %s", SDL_GetError());
        return false;
    }
    m_ttfInitialized = true;
    return true;
}

void TextRenderer::shutdown() {
    /// Frees all loaded fonts (by clearing the font map) and calls TTF_Quit
    /// if SDL_ttf was successfully initialised.
    m_fonts.clear();
    m_activeFont = nullptr;
    if (m_ttfInitialized) {
        TTF_Quit();
        m_ttfInitialized = false;
    }
}

bool TextRenderer::loadFont(const std::string &name, const std::string &path, float size) {
    /// Loads a TrueType font from disk and registers it under the given name.
    /// @param name  Key used to reference this font later (e.g. "ui", "title").
    /// @param path  Filesystem path to the .ttf file.
    /// @param size  Font size in points.
    /// @return true if the font was loaded successfully.
    TTF_Font *font = TTF_OpenFont(path.c_str(), size);
    if (!font) {
        SDL_Log("TextRenderer: Failed to load font '%s': %s", path.c_str(), SDL_GetError());
        return false;
    }
    m_fonts[name] = UniqueTTFFont(font);
    return true;
}

void TextRenderer::setActiveFont(const std::string &name) {
    /// Switches the active font. Subsequent renderText / measureText calls
    /// use this font until setActiveFont is called again.
    /// @param name  The font name as passed to loadFont.
    auto it = m_fonts.find(name);
    if (it != m_fonts.end())
        m_activeFont = it->second.get();
}

UniqueSDLTexture TextRenderer::renderText(SDL_Renderer *renderer,
                                            const std::string &text,
                                            SDL_Color color) {
    /// Renders a string to an SDL_Texture using the active font and colour.
    /// Creates an SDL_Surface via TTF_RenderText_Blended, then converts to
    /// an SDL_Texture. The caller owns the returned texture.
    /// @param renderer  SDL_Renderer to create the texture from.
    /// @param text      The string to render.
    /// @param color     Text colour (RGBA).
    /// @return A new SDL_Texture, or nullptr on failure.
    if (!m_activeFont || text.empty() || !renderer) return nullptr;

    SDL_Surface *surface = TTF_RenderText_Blended(m_activeFont, text.c_str(), text.size(), color);
    if (!surface) return nullptr;

    UniqueSDLSurface surf(surface);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf.get());
    if (!tex) return nullptr;

    return UniqueSDLTexture(tex);
}

void TextRenderer::measureText(const std::string &text, float &outW, float &outH) const {
    /// Measures the pixel dimensions the given text would occupy with the active font.
    /// Sets both outputs to 0 if the font is not set or text is empty.
    /// @param text  The string to measure.
    /// @param outW  [out] Width in pixels.
    /// @param outH  [out] Height in pixels.
    outW = 0.0f;
    outH = 0.0f;
    if (!m_activeFont || text.empty()) return;

    int w = 0, h = 0;
    if (TTF_GetStringSize(m_activeFont, text.c_str(), text.size(), &w, &h)) {
        outW = static_cast<float>(w);
        outH = static_cast<float>(h);
    }
}

} // namespace Engine
