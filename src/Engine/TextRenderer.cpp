#include "Engine/TextRenderer.hpp"

#ifndef __EMSCRIPTEN__
#include <SDL3_ttf/SDL_ttf.h>

namespace Engine {

TextRenderer::~TextRenderer() { shutdown(); }

bool TextRenderer::initialize() {
    if (!TTF_Init()) {
        SDL_Log("TextRenderer: TTF_Init failed: %s", SDL_GetError());
        return false;
    }
    m_ttfInitialized = true;
    return true;
}

void TextRenderer::shutdown() {
    m_fonts.clear();
    m_activeFont = nullptr;
    if (m_ttfInitialized) { TTF_Quit(); m_ttfInitialized = false; }
}

bool TextRenderer::loadFont(const std::string &name, const std::string &path, float size) {
    TTF_Font *font = TTF_OpenFont(path.c_str(), size);
    if (!font) { SDL_Log("TextRenderer: %s", SDL_GetError()); return false; }
    m_fonts[name] = UniqueTTFFont(font);
    return true;
}

void TextRenderer::setActiveFont(const std::string &name) {
    auto it = m_fonts.find(name);
    if (it != m_fonts.end()) m_activeFont = it->second.get();
}

UniqueSDLTexture TextRenderer::renderText(SDL_Renderer *renderer, const std::string &text, SDL_Color color) {
    if (!m_activeFont || text.empty() || !renderer) return nullptr;
    SDL_Surface *surface = TTF_RenderText_Blended(m_activeFont, text.c_str(), text.size(), color);
    if (!surface) return nullptr;
    UniqueSDLSurface surf(surface);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf.get());
    return tex ? UniqueSDLTexture(tex) : nullptr;
}

void TextRenderer::measureText(const std::string &text, float &outW, float &outH) const {
    outW = outH = 0;
    if (!m_activeFont || text.empty()) return;
    int w = 0, h = 0;
    if (TTF_GetStringSize(m_activeFont, text.c_str(), text.size(), &w, &h)) {
        outW = static_cast<float>(w); outH = static_cast<float>(h);
    }
}

} // namespace Engine

#else
// ===== WASM build: stb_truetype =====

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <fstream>

namespace Engine {

TextRenderer::~TextRenderer() { shutdown(); }

bool TextRenderer::initialize() {
    m_initialized = true;
    return true;
}

void TextRenderer::shutdown() {
    m_fonts.clear();
    m_activeFontName.clear();
    m_initialized = false;
}

bool TextRenderer::loadFont(const std::string &name, const std::string &path, float size) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    std::streamsize len = file.tellg();
    file.seekg(0, std::ios::beg);

    WASMFont font;
    font.ttfData.resize(static_cast<size_t>(len));
    if (!file.read(reinterpret_cast<char *>(font.ttfData.data()), len)) return false;
    file.close();

    if (!stbtt_InitFont(&font.info, font.ttfData.data(), 0)) return false;

    font.size = size;
    font.scale = stbtt_ScaleForPixelHeight(&font.info, size);
    int descent, lineGap;
    stbtt_GetFontVMetrics(&font.info, &font.ascent, &descent, &lineGap);

    m_fonts[name] = std::move(font);
    return true;
}

void TextRenderer::setActiveFont(const std::string &name) {
    if (m_fonts.count(name)) m_activeFontName = name;
}

UniqueSDLTexture TextRenderer::renderText(SDL_Renderer *renderer, const std::string &text, SDL_Color color) {
    auto it = m_fonts.find(m_activeFontName);
    if (it == m_fonts.end() || text.empty() || !renderer) return nullptr;

    const WASMFont &font = it->second;

    // Measure
    int totalW = 0, totalH = 0, minY = 0;
    {
        float x = 0;
        int first = true;
        for (size_t i = 0; i < text.size(); ) {
            uint32_t cp = 0;
            unsigned char c = text[i];
            if (c < 0x80) { cp = c; i += 1; }
            else if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) { cp = c & 0x1F; cp = (cp << 6) | (text[i+1] & 0x3F); i += 2; }
            else if ((c & 0xF0) == 0xE0 && i + 2 < text.size()) { cp = c & 0x0F; cp = (cp << 12) | ((text[i+1] & 0x3F) << 6) | (text[i+2] & 0x3F); i += 3; }
            else if ((c & 0xF8) == 0xF0 && i + 3 < text.size()) { cp = c & 0x07; cp = (cp << 18) | ((text[i+1] & 0x3F) << 12) | ((text[i+2] & 0x3F) << 6) | (text[i+3] & 0x3F); i += 4; }
            else { cp = c; i += 1; }

            int advance, lsb;
            stbtt_GetCodepointHMetrics(&font.info, cp, &advance, &lsb);
            x += advance * font.scale;

            if (first) {
                int descent, lineGap;
                stbtt_GetFontVMetrics(&font.info, &minY, &descent, &lineGap);
                totalH = static_cast<int>((minY - descent) * font.scale);
                first = false;
            }
        }
        totalW = static_cast<int>(std::ceil(x));
        if (totalW <= 0) return nullptr;
        if (totalH <= 0) totalH = static_cast<int>(font.size);
    }

    // Allocate pixel buffer (RGBA)
    int pitch = totalW * 4;
    std::vector<uint8_t> pixels(static_cast<size_t>(totalH) * pitch, 0);

    // Render glyphs
    float cursorXF = 0;
    int baseY = font.ascent;
    for (size_t i = 0; i < text.size(); ) {
        uint32_t cp = 0;
        unsigned char c = text[i];
        if (c < 0x80) { cp = c; i += 1; }
        else if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) { cp = c & 0x1F; cp = (cp << 6) | (text[i+1] & 0x3F); i += 2; }
        else if ((c & 0xF0) == 0xE0 && i + 2 < text.size()) { cp = c & 0x0F; cp = (cp << 12) | ((text[i+1] & 0x3F) << 6) | (text[i+2] & 0x3F); i += 3; }
        else if ((c & 0xF8) == 0xF0 && i + 3 < text.size()) { cp = c & 0x07; cp = (cp << 18) | ((text[i+1] & 0x3F) << 12) | ((text[i+2] & 0x3F) << 6) | (text[i+3] & 0x3F); i += 4; }
        else { cp = c; i += 1; }

        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font.info, cp, &advance, &lsb);

        int gw, gh, xoff, yoff;
        unsigned char *glyph = stbtt_GetCodepointBitmap(&font.info, font.scale, font.scale, cp, &gw, &gh, &xoff, &yoff);
        if (glyph) {
            int startX = static_cast<int>(cursorXF) + xoff;
            int startY = static_cast<int>(baseY * font.scale) + yoff;
            for (int row = 0; row < gh; row++) {
                int py = startY + row;
                if (py < 0 || py >= totalH) continue;
                for (int col = 0; col < gw; col++) {
                    int px = startX + col;
                    if (px < 0 || px >= totalW) continue;
                    uint8_t alpha = glyph[row * gw + col];
                    if (alpha) {
                        size_t idx = static_cast<size_t>(py) * pitch + static_cast<size_t>(px) * 4;
                        pixels[idx + 0] = color.r;
                        pixels[idx + 1] = color.g;
                        pixels[idx + 2] = color.b;
                        pixels[idx + 3] = alpha;
                    }
                }
            }
            stbtt_FreeBitmap(glyph, nullptr);
        }
        cursorXF += advance * font.scale;
    }

    SDL_Surface *surface = SDL_CreateSurfaceFrom(totalW, totalH, SDL_PIXELFORMAT_RGBA32, pixels.data(), pitch);
    if (!surface) return nullptr;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    return tex ? UniqueSDLTexture(tex) : nullptr;
}

void TextRenderer::measureText(const std::string &text, float &outW, float &outH) const {
    outW = outH = 0;
    auto it = m_fonts.find(m_activeFontName);
    if (it == m_fonts.end() || text.empty()) return;

    const WASMFont &font = it->second;
    float x = 0;
    bool first = true;
    int totalH = 0;

    for (size_t i = 0; i < text.size(); ) {
        uint32_t cp = 0;
        unsigned char c = text[i];
        if (c < 0x80) { cp = c; i += 1; }
        else if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) { cp = c & 0x1F; cp = (cp << 6) | (text[i+1] & 0x3F); i += 2; }
        else if ((c & 0xF0) == 0xE0 && i + 2 < text.size()) { cp = c & 0x0F; cp = (cp << 12) | ((text[i+1] & 0x3F) << 6) | (text[i+2] & 0x3F); i += 3; }
        else if ((c & 0xF8) == 0xF0 && i + 3 < text.size()) { cp = c & 0x07; cp = (cp << 18) | ((text[i+1] & 0x3F) << 12) | ((text[i+2] & 0x3F) << 6) | (text[i+3] & 0x3F); i += 4; }
        else { cp = c; i += 1; }

        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font.info, cp, &advance, &lsb);
        x += advance * font.scale;

        if (first) {
            int descent, lineGap;
            stbtt_GetFontVMetrics(&font.info, &totalH, &descent, &lineGap);
            totalH = static_cast<int>((totalH - descent) * font.scale);
            first = false;
        }
    }

    outW = static_cast<float>(std::ceil(x));
    outH = static_cast<float>(totalH > 0 ? totalH : font.size);
}

} // namespace Engine

#endif