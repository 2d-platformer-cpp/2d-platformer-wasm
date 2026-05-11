/// @file Renderer.cpp
/// Implementation of the Renderer class: SDL_Renderer creation, texture
/// loading/caching, draw-command dispatch, parallax tiling, and overlay drawing.

#include "Engine/Renderer.hpp"
#include "Common/Constants.hpp"
#include <algorithm>
#include <cmath>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

namespace Engine {

Renderer::Renderer(SDL_Window *window) {
    /// Creates an SDL_Renderer with VSync enabled and sets logical
    /// presentation to SCREEN_WIDTH x SCREEN_HEIGHT (letterboxed).
    /// @param window  The SDL_Window to render into.
    SDL_Renderer *r = SDL_CreateRenderer(window, NULL);
    if (!r) return;
    m_sdlRenderer.reset(r);
    SDL_SetRenderVSync(r, 1);
    SDL_SetRenderLogicalPresentation(r, Common::SCREEN_WIDTH, Common::SCREEN_HEIGHT,
                                      SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

bool Renderer::loadTexture(Common::TextureID id, const std::string &path) {
    /// Loads a PNG from disk via SDL_image and caches it under the given ID.
    /// @param id    The TextureID to store the texture under.
    /// @param path  Filesystem path to the PNG file.
    /// @return true on success, false if the renderer is missing or loading failed.
    if (!m_sdlRenderer) return false;

    auto surface = UniqueSDLSurface(IMG_Load(path.c_str()));
    if (!surface) return false;

    auto texture = UniqueSDLTexture(SDL_CreateTextureFromSurface(m_sdlRenderer.get(), surface.get()));
    if (!texture) return false;

    m_textureCache[id] = std::move(texture);
    return true;
}

void Renderer::beginFrame() {
    /// Clears the backbuffer to black. Call at the start of each frame.
    if (!m_sdlRenderer) return;
    SDL_SetRenderDrawColor(m_sdlRenderer.get(), 0, 0, 0, 255);
    SDL_RenderClear(m_sdlRenderer.get());
}

void Renderer::endFrame() {
    /// Presents the rendered frame to the screen. Call at the end of each frame.
    if (!m_sdlRenderer) return;
    SDL_RenderPresent(m_sdlRenderer.get());
}

void Renderer::drawCommands(const std::vector<Common::RenderCommand> &commands, float cameraOffsetX) {
    /// Iterates all render commands: applies parallax-aware camera offset,
    /// tiles background textures (FAR/MID/NEAR) horizontally to fill the viewport,
    /// draws other textures directly, and falls back to coloured rectangles
    /// (with per-ID colour mapping) when textures are missing or TEX_NONE.
    /// @param commands        The command list to process.
    /// @param cameraOffsetX   Current camera scroll offset.
    if (!m_sdlRenderer) return;

    for (const auto &cmd : commands) {
        SDL_FRect dest = {
            cmd.x - (cmd.scrollFactor * cameraOffsetX),
            cmd.y, cmd.width, cmd.height
        };

        auto it = m_textureCache.find(cmd.textureID);
        if (it != m_textureCache.end() && it->second) {
            if (cmd.textureID == Common::TextureID::TEX_BACKGROUND_FAR ||
                cmd.textureID == Common::TextureID::TEX_BACKGROUND_MID ||
                cmd.textureID == Common::TextureID::TEX_BACKGROUND_NEAR) {
                float texW, texH;
                SDL_GetTextureSize(it->second.get(), &texW, &texH);
                float scrollOffset = cmd.scrollFactor * cameraOffsetX;
                float startX = -std::fmod(scrollOffset, texW);
                for (int y = 0; y < Common::SCREEN_HEIGHT; y += (int)texH)
                    for (float x = startX; x < (float)Common::SCREEN_WIDTH; x += texW) {
                        SDL_FRect tileRect = {x, (float)y, texW, texH};
                        SDL_RenderTexture(m_sdlRenderer.get(), it->second.get(), NULL,
                                          &tileRect);
                    }
            } else {
                SDL_FRect srcRect = {(float)cmd.srcX, (float)cmd.srcY, (float)cmd.srcW, (float)cmd.srcH};
                bool hasSrc = cmd.srcW > 0 && cmd.srcH > 0;
                SDL_RenderTexture(m_sdlRenderer.get(), it->second.get(), hasSrc ? &srcRect : NULL, &dest);
            }
        } else {
            unsigned char r = 0, g = 255, b = 255, a = 255;
            switch (cmd.textureID) {
                case Common::TextureID::TEX_PLAYER:          r=0;   g=255; b=255; break;
                case Common::TextureID::TEX_PLATFORM:
                case Common::TextureID::TEX_FLOOR:           r=0;   g=255; b=0;   break;
                case Common::TextureID::TEX_ENEMY:           r=255; g=0;   b=0;   break;
                case Common::TextureID::TEX_BACKGROUND_FAR:  r=20;  g=20;  b=30;  break;
                case Common::TextureID::TEX_BACKGROUND_MID:  r=50;  g=50;  b=80;  break;
                case Common::TextureID::TEX_BACKGROUND_NEAR: r=80;  g=80;  b=120; break;
                case Common::TextureID::TEX_HAZARD_LAVA:     r=255; g=165; b=0;   break;
                case Common::TextureID::TEX_HAZARD_SPIKE:    r=128; g=128; b=128; break;
                case Common::TextureID::TEX_COLLECTIBLE:    r=255; g=255; b=0;   break;
                case Common::TextureID::TEX_NONE:            r=cmd.colorR; g=cmd.colorG; b=cmd.colorB; a=cmd.colorA; break;
                default: break;
            }
            if (cmd.textureID == Common::TextureID::TEX_NONE)
                SDL_SetRenderDrawBlendMode(m_sdlRenderer.get(), SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(m_sdlRenderer.get(), r, g, b, a);
            SDL_RenderFillRect(m_sdlRenderer.get(), &dest);
            if (cmd.textureID == Common::TextureID::TEX_NONE)
                SDL_SetRenderDrawBlendMode(m_sdlRenderer.get(), SDL_BLENDMODE_NONE);
        }
    }
}

void Renderer::drawUI(const std::vector<Common::RenderCommand> &commands) {
    /// Convenience wrapper: draws UI commands with no camera scrolling.
    /// @param commands  UI render commands (buttons, panels, overlays).
    drawCommands(commands, 0.0f);
}

void Renderer::drawTexture(SDL_Texture *texture, float x, float y, float w, float h) {
    /// Directly draws an SDL_Texture at the given position and size.
    /// @param texture  The SDL_Texture to draw (must not be null).
    /// @param x, y     Destination top-left.
    /// @param w, h     Destination dimensions.
    if (!m_sdlRenderer || !texture) return;
    SDL_FRect dest = {x, y, w, h};
    SDL_RenderTexture(m_sdlRenderer.get(), texture, NULL, &dest);
}

SDL_Texture *Renderer::getTexture(Common::TextureID id) const {
    /// Looks up a previously loaded texture by its TextureID.
    /// @param id  The texture identifier.
    /// @return Pointer to the SDL_Texture, or nullptr if not loaded.
    auto it = m_textureCache.find(id);
    return (it != m_textureCache.end()) ? it->second.get() : nullptr;
}

void Renderer::drawOverlay(float alpha) {
    /// Draws a full-screen semi-transparent black overlay with the given
    /// alpha (clamped 0.0 – 255.0). Restores the previous blend mode.
    /// @param alpha  Opacity from 0.0 (transparent) to 255.0 (fully opaque).
    if (!m_sdlRenderer) return;
    alpha = std::clamp(alpha, 0.0f, Common::FADE_MAX_ALPHA);
    SDL_BlendMode prev;
    SDL_GetRenderDrawBlendMode(m_sdlRenderer.get(), &prev);
    SDL_SetRenderDrawBlendMode(m_sdlRenderer.get(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_sdlRenderer.get(), 0, 0, 0, static_cast<Uint8>(alpha));
    SDL_FRect full = {0, 0, static_cast<float>(Common::SCREEN_WIDTH), static_cast<float>(Common::SCREEN_HEIGHT)};
    SDL_RenderFillRect(m_sdlRenderer.get(), &full);
    SDL_SetRenderDrawBlendMode(m_sdlRenderer.get(), prev);
}

} // namespace Engine
