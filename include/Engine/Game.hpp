/// @file Game.hpp
/// Top-level game orchestrator owning all engine and gameplay subsystems.
/// Drives the fixed-timestep game loop, state machine, and audio system.

#pragma once

#include "Engine/WindowManager.hpp"
#include "Engine/InputManager.hpp"
#include "Engine/Renderer.hpp"
#include "Engine/TextRenderer.hpp"
#include "Engine/Camera.hpp"
#include "Engine/LevelEditor.hpp"
#include "Engine/AudioManager.hpp"
#include "Gameplay/EntityManager.hpp"
#include "Gameplay/UI/MenuSystem.hpp"
#include "Gameplay/UI/ProgressIndicator.hpp"
#include "Gameplay/StatsManager.hpp"
#include "CLI/AuthService.hpp"
#include "Common/Types.hpp"
#include "Common/SFX.hpp"
#include <optional>
#include <string>
#include <vector>
#include <random>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace Engine {

/// Top-level game orchestrator owning all engine and gameplay subsystems.
/// Drives the fixed-timestep game loop: input polling, update, rendering.
/// Manages game state transitions (welcome→menu→running→paused→exiting),
/// level transitions, death/respawn flow, and the level editor.
class Game {
public:
    /// Constructs the Game; subsystems are initialised in initialize().
    Game();
    /// Default destructor; subsystems shut down via RAII.
    ~Game() = default;

    /// One-time setup: loads the level, textures, fonts, user stats, and audio.
    /// @param levelPath    Path to the initial .level file.
    /// @param enterEditor  If true, immediately enter editor mode.
    /// @param newLevelWidth  Initial level width (used when creating a new empty level in editor).
    /// @param currentUser  Authenticated user (or nullopt for guest).
    /// @param authService  Pointer to the auth service for progress/stats persistence.
    void initialize(const std::string &levelPath, bool enterEditor,
                    float newLevelWidth = Common::DEFAULT_LEVEL_WIDTH,
                    std::optional<CLI::User> currentUser = std::nullopt,
                    CLI::AuthService *authService = nullptr);

    /// Routes input based on current GameState.
    /// Handles quit, level-complete dismissal, pause toggle, and menu button clicks.
    /// @param input  Current frame input snapshot.
    void handleInput(const Common::InputState &input);

    /// Fixed-timestep simulation: physics, collisions, audio, transitions,
    /// death detection, level completion, and editor logic.
    /// @param deltaTime  Fixed time step (Common::TIME_STEP).
    /// @param input      Current frame input snapshot.
    void update(float deltaTime, const Common::InputState &input);

    /// Builds and submits all render and text commands for one frame.
    /// Includes world entities, backgrounds, HUD, menus, overlays, and death screen.
    void render();

    /// The main game loop: accumulator-based fixed timestep, input polling,
    /// update, render, and FPS calculation. In WASM builds, sets up
    /// emscripten_set_main_loop instead of a blocking while loop.
    void run();

    /// Whether the game loop should continue (state != EXITING).
    /// @return true if the game loop should keep running.
    bool isRunning() const { return m_gameState != Common::GameState::EXITING; }
    /// Whether the entire application should close (set on window close or admin exit).
    /// @return true if the application should terminate.
    bool shouldExitApp() const { return m_exitApp; }
    /// Whether the level editor has a pending "save and exit" request.
    /// @return true if the editor wants to save and exit.
    bool hasPendingSubmission() const { return m_levelEditor.hasPendingSubmission(); }
    /// Const access to the entity manager (used by main.cpp for saving on editor exit).
    /// @return Const reference to the entity manager.
    const Gameplay::EntityManager &getEntityManager() const { return m_entityManager; }

    /// Runs a single frame of the game loop (input, update, render).
    /// Returns false when the game should exit (state == EXITING).
    bool runOneFrame();

    /// Cleans up user stats and progress after the game loop ends.
    void onLoopEnd();

private:
    // --- Core subsystems ---
    Common::GameState m_gameState;                ///< Current state in the game state machine.
    WindowManager m_window;                       ///< SDL_Window ownership and fullscreen.
    InputManager m_inputManager;                  ///< Input polling.
    Renderer m_renderer;                          ///< Rendering pipeline and texture cache.
    Camera m_camera;                              ///< Horizontal scrolling camera.
    Gameplay::StatsManager m_statsManager;        ///< Per-level and cumulative statistics.
    Gameplay::EntityManager m_entityManager;      ///< All in-game entities.
    Gameplay::UI::ProgressIndicator m_progressIndicator;  ///< Top-right level progress bar.
    Engine::LevelEditor m_levelEditor;            ///< In-game level editor.
    Gameplay::UI::MenuSystem m_menuSystem;        ///< Main menu and pause menu rendering.

    // --- Auth & state ---
    std::optional<CLI::User> m_currentUser;       ///< Currently logged-in user.
    CLI::AuthService *m_authService = nullptr;    ///< Auth service (non-owning pointer).
    std::string m_currentLevel;                    ///< Path to the currently loaded level.

    // --- Transition state ---
    bool m_isTransitioning = false;                ///< Fade-to-black is in progress.
    bool m_isDeathRespawn = false;                 ///< Current transition is a death respawn.
    bool m_showLevelComplete = false;              ///< Show "Level Complete!" overlay before next level.
    float m_transitionAlpha = 0.0f;                ///< Current fade alpha (0–255).
    std::string m_pendingLevel;                    ///< Level to load after the transition.
    Gameplay::CardinalDirection m_entryDirection = Gameplay::CardinalDirection::East; ///< Player spawn side.
    float m_currentTransitionDuration = Common::DEFAULT_TRANSITION_DURATION;           ///< Fade duration for current transition.

    // --- Input debouncing ---
    bool m_pauseWasPressed = false;                ///< ESC key was held last frame.
    bool m_exitApp = false;                        ///< Signal main loop to exit entirely.
    bool m_prevMouseDown = false;                  ///< Mouse button state for edge detection.

    // --- Audio state machine ---
    bool m_prevPlayerGrounded = true;              ///< Player grounded state last frame.
    float m_runFootstepTimer = 0.0f;               ///< Accumulator for footstep SFX interval.
    float m_consecutiveRunTime = 0.0f;             ///< How long the player has been running continuously.
    bool m_heavyBreathingPlayed = false;            ///< Whether heavy breathing SFX has fired for this run.
    float m_idleTime = 0.0f;                       ///< How long the player has been standing still.
    float m_idleBreathingTimer = 0.0f;             ///< Timer for periodic idle breathing SFX.
    float m_birdsChirpTimer = 5.0f;                ///< Timer for random bird chirp SFX.
    float m_welcomeTimer = 2.0f;                   ///< Countdown before welcome screen transitions to menu.
    bool m_lavaSoundPlaying = false;                ///< Whether lava loop is currently active.

    // --- Helpers ---
    /// Copies all StatsManager data into the AuthService user record and persists to disk.
    void syncUserStats();
    /// Clears entities, loads a new level, recalculates camera bounds.
    /// Sets m_gameState to EXITING on failure.
    /// @param levelPath  Path to the .level file.
    /// @return true on success, false if loading failed.
    bool loadAndSetupLevel(const std::string &levelPath);

    // --- Render helpers (extracted from the monolithic render()) ---
    void renderWelcomeScreen();
    void renderMainMenuText();
    void renderStatsScreenPanel();
    void renderPausedText();
    void renderHUDText(const std::string &levelName);
    void renderDeathScreen();
    void renderLevelCompleteOverlay();

    void renderFrameText(const std::string &font, const std::string &text,
                         float x, float y, SDL_Color color);
    void renderFrameCentered(const std::string &font, const std::string &text,
                             float cx, float cy, SDL_Color color);
    void renderFrameTextWithShadow(const std::string &font, const std::string &text,
                                    float x, float y, SDL_Color color);

    std::string extractLevelName(const std::string &levelPath) const;

    // --- Game loop state (used by runOneFrame) ---
    Uint64 m_lastTime = 0;         ///< Timestamp of the last frame in ms.
    float m_accumulator = 0.0f;    ///< Accumulated frame time for fixed-step updates.
    Uint64 m_fpsTimer = 0;         ///< Timer for FPS counter updates.
    int m_fpsFrames = 0;           ///< Frame count since last FPS update.

    // --- Random ---
    std::mt19937 m_rng;  ///< Mersenne Twister RNG seeded from std::random_device.

    // --- Audio & text ---
    AudioManager m_audioManager;       ///< SFX playback and looping.
    TextRenderer m_textRenderer;       ///< Font management and text rendering.

    // --- Frame-local data ---
    std::vector<Common::RenderCommand> m_uiCommands;   ///< UI render commands (rebuilt each frame).
    std::vector<Common::TextCommand> m_textCommands;   ///< Text commands (rebuilt each frame).
    std::vector<UniqueSDLTexture> m_frameTextures;     ///< Temporary textures that must live for one frame.
};

} // namespace Engine
