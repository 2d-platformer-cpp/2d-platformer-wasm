/// @file stb_wasm_impl.cpp
/// Single compilation unit for stb implementation defines (WASM only).
/// Must be compiled exactly once to avoid multiple-definition errors.

#ifdef __EMSCRIPTEN__
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#endif
