PROJECT_NAME := cpp_2d_game

SRC_DIR      := src
INC_DIR      := include
BUILD_DIR    := build
ASSETS_DIR   := assets

CXX_STANDARD := -std=c++23
WARNINGS     := -Wall -Wextra -Wpedantic
DEBUG_FLAGS  := -g -O0
RELEASE_FLAGS:= -O2 -DNDEBUG

# ================================
# Platform Detection (Cross-Platform, Robust)
# ================================

UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
UNAME_M := $(shell uname -m 2>/dev/null || echo x86_64)
OS := $(shell echo $$OS 2>/dev/null || echo)

ifdef OS
  ifeq ($(OS),Windows_NT)
    PLATFORM := windows
    CXX := g++
  endif
endif

ifeq ($(PLATFORM),)
  ifeq ($(UNAME_S),Darwin)
    PLATFORM := macos
    CXX := clang++
  else ifeq ($(UNAME_S),Linux)
    PLATFORM := linux
    CXX := g++
  else
    PLATFORM := unknown
    CXX := g++
    $(warning Unknown platform: $(UNAME_S). Defaulting to generic Unix build.)
  endif
endif

PLATFORM ?= $(PLATFORM)

$(info Platform detected: $(PLATFORM))
$(info Architecture: $(UNAME_M))

# ================================
# Portable Recursive Wildcard (No Shell Commands)
# Only matches directories in the foreach to avoid including files as dirs
# ================================

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*/),$(call rwildcard,$d,$2))

SRC_FILES := $(call rwildcard,$(SRC_DIR)/,*.cpp)

# ================================
# SDL3 Configuration Per Platform
# ================================

SDL3_BASE := lib/SDL3
SDL3_FOUND := no

ifeq ($(PLATFORM),windows)
    SDL3_PATH := $(SDL3_BASE)/x86_64-w64-mingw32
    ifneq ($(wildcard $(SDL3_PATH)/include/SDL3/SDL.h),)
        SDL3_INC := -I$(SDL3_PATH)/include
        SDL3_LIB := -L$(SDL3_PATH)/lib -lSDL3 -lSDL3_image
        SDL3_DLLS := $(SDL3_PATH)/bin/SDL3.dll $(SDL3_PATH)/bin/SDL3_image.dll
        SDL3_FOUND := yes
        $(info Using bundled SDL3 for Windows)
    else
        $(info ERROR: Bundled SDL3 not found at $(SDL3_PATH))
        $(info Windows requires bundled SDL3 in lib/SDL3/x86_64-w64-mingw32/)
    endif
    TARGET_EXT := .exe

else ifeq ($(PLATFORM),macos)
    ifneq ($(wildcard $(SDL3_BASE)/macos/include/SDL3/SDL.h),)
        SDL3_INC := -I$(SDL3_BASE)/macos/include
        SDL3_LIB := -L$(SDL3_BASE)/macos/lib -lSDL3 -lSDL3_image -framework Cocoa
        SDL3_FOUND := yes
        $(info Using bundled SDL3 for macOS)
    else ifneq ($(shell pkg-config --exists sdl3 2>/dev/null && echo yes),)
        SDL3_FOUND := yes
        SDL3_INC := $(shell pkg-config --cflags sdl3)
        SDL3_LIB := $(shell pkg-config --libs sdl3)
        $(info Using system SDL3 via pkg-config (macOS))
        ifneq ($(shell pkg-config --exists sdl3-image 2>/dev/null && echo yes),)
            SDL3_INC += $(shell pkg-config --cflags sdl3-image)
            SDL3_LIB += $(shell pkg-config --libs sdl3-image)
        else
            $(warning sdl3-image not found; some features may be unavailable)
        endif
        ifneq ($(shell pkg-config --exists sdl3-ttf 2>/dev/null && echo yes),)
            SDL3_INC += $(shell pkg-config --cflags sdl3-ttf)
            SDL3_LIB += $(shell pkg-config --libs sdl3-ttf)
            $(info Using system SDL3_ttf via pkg-config)
        else
            $(warning sdl3-ttf not found; text rendering will be disabled)
        endif
    else
        $(info SDL3 not found via pkg-config)
    endif
    TARGET_EXT :=

else ifeq ($(PLATFORM),linux)
    ifneq ($(wildcard $(SDL3_BASE)/linux/include/SDL3/SDL.h),)
        SDL3_INC := -I$(SDL3_BASE)/linux/include
        SDL3_LIB := -L$(SDL3_BASE)/linux/lib -lSDL3 -lSDL3_image
        SDL3_FOUND := yes
        $(info Using bundled SDL3 for Linux)
    else ifneq ($(shell pkg-config --exists sdl3 2>/dev/null && echo yes),)
        SDL3_FOUND := yes
        SDL3_INC := $(shell pkg-config --cflags sdl3)
        SDL3_LIB := $(shell pkg-config --libs sdl3)
        $(info Using system SDL3 via pkg-config (Linux))
        ifneq ($(shell pkg-config --exists sdl3-image 2>/dev/null && echo yes),)
            SDL3_INC += $(shell pkg-config --cflags sdl3-image)
            SDL3_LIB += $(shell pkg-config --libs sdl3-image)
        else
            $(warning sdl3-image not found; some features may be unavailable)
        endif
        ifneq ($(shell pkg-config --exists sdl3-ttf 2>/dev/null && echo yes),)
            SDL3_INC += $(shell pkg-config --cflags sdl3-ttf)
            SDL3_LIB += $(shell pkg-config --libs sdl3-ttf)
            $(info Using system SDL3_ttf via pkg-config)
        else
            $(warning sdl3-ttf not found; text rendering will be disabled)
        endif
    else ifneq ($(wildcard /usr/include/SDL3/SDL.h),)
        SDL3_FOUND := yes
        SDL3_INC := -I/usr/include/SDL3
        SDL3_LIB := -lSDL3 -lSDL3_image -lSDL3_ttf
        $(info Using system SDL3 (headers found at /usr/include/SDL3/))
    else
        $(info SDL3 not found via pkg-config or system headers)
    endif
    TARGET_EXT :=

else
    $(error Unsupported platform: $(PLATFORM))
endif

PLATFORM_LIBS := $(SDL3_LIB)
PLATFORM_INCLUDES := $(SDL3_INC)

# ================================
# Google Test Configuration
# ================================
GTEST_BASE := lib/googletest
GTEST_INC := -I$(GTEST_BASE)/include -I$(GTEST_BASE)/googletest
GTEST_SRC := $(GTEST_BASE)/src/gtest-all.cc
GTEST_MAIN_SRC := $(GTEST_BASE)/src/gtest_main.cc
GTEST_OBJ := $(BUILD_DIR)/gtest.o
GTEST_MAIN_OBJ := $(BUILD_DIR)/gtest_main.o
GTEST_FOUND := no

# Test Configuration
TEST_DIR := tests
TEST_SRC := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJ := $(TEST_SRC:$(TEST_DIR)/%.cpp=$(BUILD_DIR)/$(TEST_DIR)/%.o)
TEST_TARGET := $(BUILD_DIR)/run_tests

# Game objects for testing (exclude main.cpp to avoid duplicate main with gtest)
GAME_SRC_FILES := $(filter-out %/main.cpp, $(SRC_FILES))
GAME_OBJ_FILES := $(GAME_SRC_FILES:%.cpp=$(BUILD_DIR)/%.o)

# Detect bundled Google Test
ifneq ($(wildcard $(GTEST_SRC)),)
    GTEST_FOUND := yes
    $(info Using bundled Google Test)
else
    $(warning Google Test not found. Ensure it is bundled in $(GTEST_BASE))
endif

TARGET := $(BUILD_DIR)/$(PROJECT_NAME)$(TARGET_EXT)

# ================================
# Compiler Flags
# ================================

CXXFLAGS := $(CXX_STANDARD) \
            $(WARNINGS) \
            $(DEBUG_FLAGS) \
            -I$(INC_DIR) \
            -Ilib \
            $(PLATFORM_INCLUDES) \
            -MMD -MP

# ================================
# Object Files
# ================================

OBJ_FILES := $(SRC_FILES:%.cpp=$(BUILD_DIR)/%.o)

# ================================
# Default Target
# ================================

all: wasm

# ================================
# Dependency Check
# ================================

.PHONY: check-deps

check-deps:
	@echo "Checking SDL3 dependencies..."
ifeq ($(SDL3_FOUND),yes)
	@echo "✅ SDL3 found ($(if $(findstring bundled,$(SDL3_INC)),bundled,system))"
else
	@echo "❌ SDL3 core library not found!"
	@exit 1
endif
	@echo "Checking Google Test..."
ifeq ($(GTEST_FOUND),yes)
	@echo "✅ Google Test found"
else
	@echo "❌ Google Test not found!"
	@exit 1
endif

# ================================
# Build Rules
# ================================

$(TARGET): $(OBJ_FILES)
	@echo "Linking executable"
	@$(CXX) $(OBJ_FILES) -o $@ $(PLATFORM_LIBS)
	@echo "Build Successful!"

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# ================================
# Google Test Compilation
# ================================

$(GTEST_OBJ): $(GTEST_SRC)
	@mkdir -p $(dir $@)
	@echo "Compiling Google Test"
	@$(CXX) $(CXXFLAGS) -I$(GTEST_BASE)/include -I$(GTEST_BASE) -c $< -o $@

$(GTEST_MAIN_OBJ): $(GTEST_MAIN_SRC)
	@mkdir -p $(dir $@)
	@echo "Compiling Google Test Main"
	@$(CXX) $(CXXFLAGS) -I$(GTEST_BASE)/include -I$(GTEST_BASE) -c $< -o $@

# ================================
# Test File Compilation
# ================================

$(BUILD_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling test: $<"
	@$(CXX) $(CXXFLAGS) $(GTEST_INC) -c $< -o $@

# ================================
# Directory Setup
# ================================

directories:
	@echo "Creating build directory"
	@mkdir -p $(BUILD_DIR)

# ================================
# Assets & Runtime Dependencies
# ================================

copy_runtime_deps: directories
ifeq ($(PLATFORM),windows)
	@echo "Copying SDL3 DLLs to build directory..."
	@mkdir -p $(BUILD_DIR)/bin
	@cp -f $(SDL3_PATH)/bin/SDL3.dll $(BUILD_DIR)/ 2>/dev/null || echo "Warning: SDL3.dll not found at $(SDL3_PATH)/bin/SDL3.dll"
	@cp -f $(SDL3_PATH)/bin/SDL3_image.dll $(BUILD_DIR)/ 2>/dev/null || echo "Warning: SDL3_image.dll not found at $(SDL3_PATH)/bin/SDL3_image.dll"
	@echo "Windows SDL3 DLLs copied successfully"
else
	@echo "No runtime DLL copy needed for $(PLATFORM)"
endif

copy_assets: copy_runtime_deps
	@echo "Copying assets..."
	@mkdir -p $(BUILD_DIR)/$(ASSETS_DIR)
	@mkdir -p $(BUILD_DIR)/$(ASSETS_DIR)/levels
	@mkdir -p $(BUILD_DIR)/$(ASSETS_DIR)/sprites
	@mkdir -p $(BUILD_DIR)/$(ASSETS_DIR)/audio
	@mkdir -p $(BUILD_DIR)/$(ASSETS_DIR)/users
	@mkdir -p $(BUILD_DIR)/devtools
	@if [ -d "$(ASSETS_DIR)" ] && [ "$$(ls -A $(ASSETS_DIR) 2>/dev/null)" ]; then \
		cp -r $(ASSETS_DIR)/* $(BUILD_DIR)/$(ASSETS_DIR)/ 2>/dev/null || true; \
		echo "Assets copied successfully"; \
	else \
		echo "Warning: assets folder empty or missing - creating empty structure"; \
	fi

# ================================
# Test Runner Linking
# ================================

$(TEST_TARGET): $(TEST_OBJ) $(GAME_OBJ_FILES) $(GTEST_OBJ) $(GTEST_MAIN_OBJ)
	@echo "Linking test runner"
	@$(CXX) $^ -o $@ $(PLATFORM_LIBS)

# ================================
# Test
# ================================

test: check-deps $(TEST_TARGET)
	@echo "============================"
	@echo "Running Tests..."
	@echo "============================"
	@$(TEST_TARGET)

# ================================
# Run
# ================================

run: all
	$(TARGET)

# ================================
# Release Build
# ================================

release: CXXFLAGS := $(CXX_STANDARD) $(WARNINGS) $(RELEASE_FLAGS) -I$(INC_DIR) -Ilib $(PLATFORM_INCLUDES) -MMD -MP
release: clean all

# ================================
# Clean
# ================================

clean:
	@echo "Cleaning build directory"
	@rm -rf $(BUILD_DIR)
	@echo "Build directory cleaned"

# ================================
# WASM / Emscripten Build
# ================================

WASM_BUILD_DIR := $(BUILD_DIR)/wasm
EMCC := em++

wasm: $(WASM_BUILD_DIR)/index.html

$(WASM_BUILD_DIR)/index.html: $(SRC_FILES) shell.html
	@mkdir -p $(WASM_BUILD_DIR)
	$(EMCC) -std=c++23 \
		-I$(INC_DIR) \
		-s USE_SDL=3 \
		-s INITIAL_MEMORY=67108864 \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s FORCE_FILESYSTEM=1 \
		-s MAX_WEBGL_VERSION=2 \
		-s ASYNCIFY \
		-s DISABLE_EXCEPTION_CATCHING=0 \
		-O2 \
		--shell-file shell.html \
		--preload-file assets \
		-o $@ \
		$(SRC_FILES)
	@echo "WASM build complete: $@"

DEPS := $(OBJ_FILES:.o=.d) $(TEST_OBJ:.o=.d) $(GTEST_OBJ:.o=.d) $(GTEST_MAIN_OBJ:.o=.d)
-include $(DEPS)

.PHONY: all clean run release test copy_assets directories copy_runtime_deps check-deps
