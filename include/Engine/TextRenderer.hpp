/// @file TextRenderer.hpp
/// Font management, text rasterization, and measurement.
/// In native builds uses SDL3_ttf; in WASM builds uses stb_truetype.

#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "Common/SDLDeleters.hpp"

#ifndef __EMSCRIPTEN__
// Uses SDL_ttf types from SDLDeleters.hpp
#else
#include "stb/stb_truetype.h"
#endif

namespace Engine {

/// Font management, text rasterization, and measurement.
/// Allocates SDL_Textures on demand; caller must keep them alive for the frame.
class TextRenderer {
public:
    TextRenderer() = default;
    ~TextRenderer();

    TextRenderer(const TextRenderer &) = delete;
    TextRenderer &operator=(const TextRenderer &) = delete;

    bool initialize();
    void shutdown();

    bool loadFont(const std::string &name, const std::string &path, float size);
    void setActiveFont(const std::string &name);

    UniqueSDLTexture renderText(SDL_Renderer *renderer,
                                 const std::string &text,
                                 SDL_Color color);

    void measureText(const std::string &text, float &outW, float &outH) const;

private:
#ifndef __EMSCRIPTEN__
    std::unordered_map<std::string, UniqueTTFFont> m_fonts;
    TTF_Font *m_activeFont = nullptr;
    bool m_ttfInitialized = false;
#else
    struct WASMFont {
        std::vector<uint8_t> ttfData;
        stbtt_fontinfo info;
        float size = 0.0f;
        float scale = 1.0f;
        int ascent = 0;
    };
    std::unordered_map<std::string, WASMFont> m_fonts;
    std::string m_activeFontName;
    bool m_initialized = false;


#endif
};

} // namespace Engine
