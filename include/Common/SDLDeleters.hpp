/// @file SDLDeleters.hpp
/// Custom deleters for SDL objects used with std::unique_ptr (RAII).
/// Also defines type aliases for common unique_ptr + deleter combinations.

#pragma once

#include <memory>
#include <SDL3/SDL.h>

#ifndef __EMSCRIPTEN__
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#endif

/// Custom deleter for SDL_Window: calls SDL_DestroyWindow.
struct SDLWindowDeleter {
    /// Calls SDL_DestroyWindow on the given pointer.
    /// @param w  SDL_Window pointer to destroy (may be null).
    void operator()(SDL_Window *w) const noexcept { if (w) SDL_DestroyWindow(w); }
};

/// Custom deleter for SDL_Renderer: calls SDL_DestroyRenderer.
struct SDLRendererDeleter {
    /// Calls SDL_DestroyRenderer on the given pointer.
    /// @param r  SDL_Renderer pointer to destroy (may be null).
    void operator()(SDL_Renderer *r) const noexcept { if (r) SDL_DestroyRenderer(r); }
};

/// Custom deleter for SDL_Texture: calls SDL_DestroyTexture.
struct SDLTextureDeleter {
    /// Calls SDL_DestroyTexture on the given pointer.
    /// @param t  SDL_Texture pointer to destroy (may be null).
    void operator()(SDL_Texture *t) const noexcept { if (t) SDL_DestroyTexture(t); }
};

/// Custom deleter for SDL_Surface: calls SDL_DestroySurface.
struct SDLSurfaceDeleter {
    /// Calls SDL_DestroySurface on the given pointer.
    /// @param s  SDL_Surface pointer to destroy (may be null).
    void operator()(SDL_Surface *s) const noexcept { if (s) SDL_DestroySurface(s); }
};

/// RAII wrappers using std::unique_ptr with the custom deleters above.
using UniqueSDLWindow   = std::unique_ptr<SDL_Window, SDLWindowDeleter>;
using UniqueSDLRenderer = std::unique_ptr<SDL_Renderer, SDLRendererDeleter>;
using UniqueSDLTexture  = std::unique_ptr<SDL_Texture, SDLTextureDeleter>;
using UniqueSDLSurface  = std::unique_ptr<SDL_Surface, SDLSurfaceDeleter>;

#ifndef __EMSCRIPTEN__
/// Custom deleter for TTF_Font: calls TTF_CloseFont.
struct TTF_FontDeleter {
    void operator()(TTF_Font *f) const noexcept { if (f) TTF_CloseFont(f); }
};
using UniqueTTFFont = std::unique_ptr<TTF_Font, TTF_FontDeleter>;
#endif
