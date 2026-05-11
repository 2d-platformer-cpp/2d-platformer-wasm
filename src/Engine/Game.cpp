/// @file Game.cpp
/// Implementation of the Game class: state machine, fixed-timestep game loop,
/// input routing, physics update, audio management, rendering orchestration,
/// level transitions, and death/respawn flow.

#include "Engine/Game.hpp"
#include "Gameplay/LevelLoader.hpp"
#include "Common/Colors.hpp"
#include <SDL3/SDL.h>
#include <cmath>

namespace Engine {

Game::Game()
    /// Constructs the Game; initialises all member subsystems in the
    /// initialiser list. Subsystem initialisation order:
    /// game state → window → renderer → progress bar → level editor → RNG.
    : m_gameState(Common::GameState::WELCOME)
    , m_window(Common::WINDOW_TITLE, Common::MINIMUM_SCREEN_WIDTH, Common::MINIMUM_SCREEN_HEIGHT)
    , m_renderer(m_window.get())
    , m_progressIndicator(
          Common::SCREEN_WIDTH - Common::PROGRESS_BAR_WIDTH - Common::PROGRESS_BAR_MARGIN,
          Common::PROGRESS_BAR_MARGIN,
          Common::PROGRESS_BAR_WIDTH,
          Common::PROGRESS_BAR_HEIGHT)
    , m_levelEditor(m_entityManager, m_camera)
    , m_rng(std::random_device{}())
{
}

void Game::initialize(const std::string &levelPath, bool enterEditor,
                       float newLevelWidth,
                       std::optional<CLI::User> currentUser,
                       CLI::AuthService *authService) {
    /// One-time setup: stores auth state, loads the level (or creates an
    /// empty one for the editor), loads background textures and fonts,
    /// restores user stats from persistent storage, and positions the player
    /// at the saved position if resuming.
    /// @param levelPath    Path to the initial .level file.
    /// @param enterEditor  If true, immediately enter editor mode.
    /// @param newLevelWidth  Initial level width for new editor levels.
    /// @param currentUser  Authenticated user (or nullopt for guest).
    /// @param authService  Pointer to the auth service for persistence.
    m_currentUser = currentUser;
    m_authService = authService;
    m_currentLevel = levelPath;
    m_levelEditor.setActive(enterEditor);

    if (enterEditor) {
        m_gameState = Common::GameState::RUNNING;
    }

    if (enterEditor && levelPath.empty()) {
        m_entityManager.levelConfig.isLeftWallClamped = true;
        m_entityManager.levelConfig.isRightWallClamped = true;
        m_entityManager.levelConfig.levelWidth = newLevelWidth;
    } else if (!Gameplay::LevelLoader::loadLevel(m_entityManager, levelPath)) {
        printf("ERROR: Failed to load level: %s\n", levelPath.c_str());
        printf("Starting with default level config.\n");
        m_entityManager.levelConfig.isLeftWallClamped = true;
        m_entityManager.levelConfig.isRightWallClamped = true;
        m_entityManager.levelConfig.levelWidth = Common::DEFAULT_LEVEL_WIDTH;
    }

    m_camera.setBounds(0.0f, std::max(0.0f, m_entityManager.levelConfig.levelWidth - Common::VIEWPORT_WIDTH));

    m_renderer.loadTexture(Common::TextureID::TEX_BACKGROUND_FAR, "assets/sprites/Background_Far.png");
    m_renderer.loadTexture(Common::TextureID::TEX_BACKGROUND_MID, "assets/sprites/Background_Mid_Start.png");
    m_renderer.loadTexture(Common::TextureID::TEX_BACKGROUND_NEAR, "assets/sprites/Background_Near_Start.png");
    m_renderer.loadTexture(Common::TextureID::TEX_SKULL, "assets/sprites/skull.png");
    m_renderer.loadTexture(Common::TextureID::TEX_MENU_BG, "assets/sprites/menu_bg.png");

    m_textRenderer.initialize();
    m_textRenderer.loadFont("default", "assets/fonts/Supreme-Regular.ttf", 28.0f);
    m_textRenderer.loadFont("title", "assets/fonts/Supreme-Bold.ttf", 48.0f);
    m_textRenderer.loadFont("ui", "assets/fonts/Supreme-Regular.ttf", 16.0f);
    m_textRenderer.loadFont("ui-bold", "assets/fonts/Supreme-Medium.ttf", 16.0f);
    m_textRenderer.loadFont("mono", "assets/fonts/Supreme-Light.ttf", 16.0f);
    m_textRenderer.loadFont("hud-timer", "assets/fonts/Supreme-Regular.ttf", 24.0f);
    m_textRenderer.loadFont("hud-label", "assets/fonts/Supreme-Medium.ttf", 24.0f);
    m_textRenderer.setActiveFont("default");

    m_statsManager.onLevelStart(levelPath);
    if (m_currentUser && m_authService) {
        m_authService->loadUserStatsFromFile(m_currentUser->username);
        auto *stats = m_authService->getUserStats(m_currentUser->username);
        if (stats) {
            m_statsManager.setTotalDeaths(stats->totalDeaths);
            m_statsManager.setTotalPlaytime(stats->totalPlaytime);
            m_statsManager.setLevelsCompleted(stats->levelsCompleted);
            m_statsManager.setCompletedLevels(stats->completedLevels);
            m_statsManager.setBestTimes(stats->bestTimes);
        }
    }
    m_welcomeTimer = 2.0f;

    auto player = m_entityManager.getPlayer();
    if (player && !enterEditor) {
        if (m_currentUser && (m_currentUser->posX != 0.0f || m_currentUser->posY != 0.0f)) {
            player->transform.x = m_currentUser->posX;
            player->transform.y = m_currentUser->posY;
        }
        m_camera.update(player->transform.x);
    }
}

void Game::handleInput(const Common::InputState &input) {
    /// Routes input based on current GameState. Handles quit, welcome-screen
    /// skip, level-complete dismissal, pause toggle, menu button clicks
    /// (main menu, pause, stats), and window fullscreen toggles.
    /// @param input  Current frame input snapshot.
    m_uiCommands.clear();
    m_textCommands.clear();

    m_window.update(input);

    if (input.quit) {
        syncUserStats();
        m_gameState = Common::GameState::EXITING;
        m_exitApp = true;
        return;
    }

    if (m_gameState == Common::GameState::WELCOME) {
        return;
    }

    if (m_gameState == Common::GameState::LEVEL_COMPLETE) {
        bool anyInput = input.up || input.down || input.left || input.right || input.jump
                     || input.attack || input.pause || input.enter || input.mouseLeftDown;
        if (anyInput) {
            if (!loadAndSetupLevel(m_pendingLevel)) return;
            auto player = m_entityManager.getPlayer();
            if (player) {
                if (m_entryDirection == Gameplay::CardinalDirection::East)
                    player->transform.x = Common::SPAWN_EAST_OFFSET;
                else if (m_entryDirection == Gameplay::CardinalDirection::West)
                    player->transform.x = m_entityManager.levelConfig.levelWidth
                                         - player->transform.width - Common::SPAWN_WEST_OFFSET;

                if (m_currentUser && m_authService)
                    m_authService->updateProgress(m_currentUser->username, m_currentLevel,
                                                  player->transform.x, player->transform.y);

                m_camera.update(player->transform.x);
            }
            m_audioManager.playSFX(Common::SFX::RESPAWN);
            m_statsManager.onLevelStart(m_currentLevel);
            m_isTransitioning = false;
            m_showLevelComplete = false;
            m_transitionAlpha = 0.0f;
            m_gameState = Common::GameState::RUNNING;
        }
        return;
    }

    if (input.pause && !m_pauseWasPressed) {
        if (m_gameState == Common::GameState::RUNNING)
            m_gameState = Common::GameState::PAUSED;
        else if (m_gameState == Common::GameState::PAUSED)
            m_gameState = Common::GameState::RUNNING;
        else if (m_gameState == Common::GameState::STATS_SCREEN)
            m_gameState = Common::GameState::MAIN_MENU;
    }
    m_pauseWasPressed = input.pause;

    // Hover update for states with interactive buttons
    if (m_gameState == Common::GameState::MAIN_MENU
        || m_gameState == Common::GameState::PAUSED
        || m_gameState == Common::GameState::STATS_SCREEN) {
        m_menuSystem.updateHover(input, Common::TIME_STEP);
    }

    if (m_gameState == Common::GameState::MAIN_MENU) {
        int action = m_menuSystem.updateMainMenu(input, m_uiCommands, m_textCommands);
        if (action == 1) {
            m_gameState = Common::GameState::RUNNING;
            m_prevMouseDown = false;
            m_audioManager.startLoop(Common::SFX::WIND_GRASS);
        }
        else if (action == 2) { m_gameState = Common::GameState::STATS_SCREEN; m_prevMouseDown = false; }
        else if (action == 3) m_gameState = Common::GameState::EXITING;
    } else if (m_gameState == Common::GameState::STATS_SCREEN) {
        bool justClicked = input.mouseLeftDown && !m_prevMouseDown;
        m_prevMouseDown = input.mouseLeftDown;

        float px = (Common::SCREEN_WIDTH - Common::PAUSE_PANEL_WIDTH) / 2.0f;
        float py = (Common::SCREEN_HEIGHT - Common::PAUSE_PANEL_HEIGHT) / 2.0f;
        float bx = px + (Common::PAUSE_PANEL_WIDTH - Common::MENU_BUTTON_WIDTH) / 2.0f;
        float by = py + Common::PAUSE_PANEL_HEIGHT - Common::MENU_BUTTON_HEIGHT - 20;

        if (justClicked && m_menuSystem.isButtonClicked(input.mouseX, input.mouseY, bx, by,
            Common::MENU_BUTTON_WIDTH, Common::MENU_BUTTON_HEIGHT)) {
            m_gameState = Common::GameState::MAIN_MENU;
            m_prevMouseDown = false;
        }
    } else if (m_gameState == Common::GameState::PAUSED) {
        int action = m_menuSystem.updatePauseMenu(input, m_uiCommands, m_textCommands);
        if (action == 1) { m_gameState = Common::GameState::RUNNING; m_prevMouseDown = false; }
        else if (action == 2) { m_gameState = Common::GameState::MAIN_MENU; m_prevMouseDown = false; }
        else if (action == 3) m_gameState = Common::GameState::EXITING;
    }
}

void Game::update(float deltaTime, const Common::InputState &input) {
    /// Fixed-timestep simulation: advances welcome timer, runs level-editor
    /// logic, steps physics and collisions, manages audio state (running
    /// footsteps, landing, idle breathing, lava proximity), detects death
    /// (hazard or fall) and level-exit triggers, and drives fade-to-black
    /// transitions for death respawns and inter-level travel.
    /// @param deltaTime  Fixed time step (Common::TIME_STEP).
    /// @param input      Current frame input snapshot.
    if (m_gameState == Common::GameState::WELCOME) {
        m_welcomeTimer -= deltaTime;
        if (m_welcomeTimer <= 0.0f)
            m_gameState = Common::GameState::MAIN_MENU;
        return;
    }

    if (m_gameState != Common::GameState::RUNNING) return;

    if (m_levelEditor.isActive()) {
        m_levelEditor.update(deltaTime, input);
        if (hasPendingSubmission()) m_gameState = Common::GameState::EXITING;
    }

    if (!m_isTransitioning) {
        m_statsManager.update(deltaTime);
        m_entityManager.update(deltaTime, input);
        auto player = m_entityManager.getPlayer();
        if (player) m_progressIndicator.update(player->transform.x, m_entityManager.levelConfig.levelWidth);

        auto exitData = m_entityManager.checkCollisions();

        if (player && !m_levelEditor.isActive()) {
            bool grounded = player->physics ? player->physics->isGrounded : true;
            bool justLanded = !m_prevPlayerGrounded && grounded;
            bool isRunning = grounded && player->physics && std::abs(player->physics->velocityX) > 5.0f;
            bool isIdle = grounded && player->physics && std::abs(player->physics->velocityX) <= 5.0f;

            if (justLanded)
                m_audioManager.playSFX(Common::SFX::LANDING);

            if (isRunning) {
                m_runFootstepTimer += deltaTime;
                if (m_runFootstepTimer >= 0.3f) {
                    m_audioManager.playSFX(Common::SFX::RUN_LOOP);
                    m_runFootstepTimer -= 0.3f;
                }
                m_consecutiveRunTime += deltaTime;
            } else {
                m_runFootstepTimer = 0.0f;
            }

            if (isIdle) {
                m_idleTime += deltaTime;

                if (m_consecutiveRunTime >= 3.0f && !m_heavyBreathingPlayed
                    && m_idleTime >= 0.3f) {
                    m_audioManager.playSFX(Common::SFX::HEAVY_BREATHING);
                    m_heavyBreathingPlayed = true;
                }

                if (m_idleTime >= 3.0f) {
                    m_idleBreathingTimer -= deltaTime;
                    if (m_idleBreathingTimer <= 0.0f) {
                        m_audioManager.playSFX(Common::SFX::IDLE_BREATHING);
                        std::uniform_real_distribution<float> idleDist(4.0f, 8.0f);
                        m_idleBreathingTimer = idleDist(m_rng);
                    }
                }
            } else if (grounded) {
                m_consecutiveRunTime = 0.0f;
                m_heavyBreathingPlayed = false;
                m_idleTime = 0.0f;
                m_idleBreathingTimer = 0.0f;
            }

            bool nearLava = false;
            for (const auto &entity : m_entityManager.getEntities()) {
                if (entity->collider && entity->collider->isHazard) {
                    float dist = std::abs(player->transform.x - entity->transform.x);
                    if (dist < 300.0f) {
                        nearLava = true;
                        break;
                    }
                }
            }
            if (nearLava && !m_lavaSoundPlaying) {
                m_audioManager.startLoop(Common::SFX::LAVA);
                m_lavaSoundPlaying = true;
            } else if (!nearLava && m_lavaSoundPlaying) {
                m_audioManager.stopLoop(Common::SFX::LAVA);
                m_lavaSoundPlaying = false;
            }

            m_prevPlayerGrounded = grounded;
        }

        if (m_entityManager.isPlayerDead() && !m_levelEditor.isActive()) {
            m_statsManager.onDeath();
            syncUserStats();
            m_audioManager.playSFX(Common::SFX::DEATH_SCREAM);
            m_audioManager.stopLoop(Common::SFX::LAVA);
            m_lavaSoundPlaying = false;

            m_entityManager.resetDeathFlag();
            m_isTransitioning = true;
            m_isDeathRespawn = true;
            m_pendingLevel = m_currentLevel;
            m_currentTransitionDuration = Common::DEATH_FADE_DURATION;
            m_transitionAlpha = 0.0f;
        } else if (exitData.has_value() && !m_levelEditor.isActive()) {
            {
                bool wasFirstCompletion = !m_statsManager.hasCompleted(m_currentLevel);
                m_statsManager.onLevelComplete(m_currentLevel);
                m_showLevelComplete = wasFirstCompletion;
                syncUserStats();
            }
            m_audioManager.stopLoop(Common::SFX::LAVA);
            m_lavaSoundPlaying = false;

            m_isTransitioning = true;
            m_pendingLevel = exitData->nextLevelPath;
            m_entryDirection = exitData->direction;
            m_currentTransitionDuration = exitData->transitionDuration;
        }
    } else {
        float fadeSpeed = (m_currentTransitionDuration > 0.0f)
            ? (Common::FADE_MAX_ALPHA / m_currentTransitionDuration) : 500.0f;
        m_transitionAlpha += fadeSpeed * deltaTime;

        if (m_transitionAlpha >= Common::FADE_MAX_ALPHA) {
            if (m_isDeathRespawn) {
                if (!loadAndSetupLevel(m_pendingLevel)) return;
                auto player = m_entityManager.getPlayer();
                if (player) {
                    m_camera.update(player->transform.x);
                }
                m_audioManager.playSFX(Common::SFX::RESPAWN);

                m_prevPlayerGrounded = true;
                m_runFootstepTimer = 0.0f;
                m_consecutiveRunTime = 0.0f;
                m_heavyBreathingPlayed = false;
                m_idleTime = 0.0f;
                m_idleBreathingTimer = 0.0f;
                m_lavaSoundPlaying = false;

                m_isDeathRespawn = false;
                m_isTransitioning = false;
                m_transitionAlpha = 0.0f;
            } else if (m_showLevelComplete) {
                m_gameState = Common::GameState::LEVEL_COMPLETE;
                m_isTransitioning = false;
            } else {
                if (!loadAndSetupLevel(m_pendingLevel)) return;
                auto player = m_entityManager.getPlayer();
                if (player) {
                    if (m_entryDirection == Gameplay::CardinalDirection::East)
                        player->transform.x = Common::SPAWN_EAST_OFFSET;
                    else if (m_entryDirection == Gameplay::CardinalDirection::West)
                        player->transform.x = m_entityManager.levelConfig.levelWidth
                                             - player->transform.width - Common::SPAWN_WEST_OFFSET;

                    if (m_currentUser && m_authService) {
                        m_authService->updateProgress(m_currentUser->username, m_currentLevel,
                                                      player->transform.x, player->transform.y);
                        m_authService->saveUsersToFile();
                    }

                    m_camera.update(player->transform.x);
                }

                m_audioManager.playSFX(Common::SFX::RESPAWN);

                m_statsManager.onLevelStart(m_currentLevel);
                m_isTransitioning = false;
                m_showLevelComplete = false;
                m_transitionAlpha = 0.0f;
                m_gameState = Common::GameState::RUNNING;
            }
        }
    }

    m_audioManager.update();

    if (m_gameState == Common::GameState::RUNNING && !m_levelEditor.isActive()) {
        m_birdsChirpTimer -= deltaTime;
        if (m_birdsChirpTimer <= 0.0f) {
            m_audioManager.playSFX(Common::SFX::BIRDS_CHIRP);
            std::uniform_real_distribution<float> birdDist(5.0f, 15.0f);
            m_birdsChirpTimer = birdDist(m_rng);
        }
    }

    auto player = m_entityManager.getPlayer();
    if (player && !m_levelEditor.isActive())
        m_camera.update(player->transform.x);
}

void Game::render() {
    m_frameTextures.clear();
    std::vector<Common::RenderCommand> frameCommands;
    m_renderer.beginFrame();

    std::string levelName = extractLevelName(m_statsManager.getCurrentLevel());

    if (m_gameState == Common::GameState::WELCOME || m_gameState == Common::GameState::MAIN_MENU) {
        frameCommands.push_back({0, 0, static_cast<float>(Common::SCREEN_WIDTH), static_cast<float>(Common::SCREEN_HEIGHT),
                                  Common::TextureID::TEX_MENU_BG, 0.0f, 0, 0, 0, 0});
        frameCommands.push_back({0, 0, static_cast<float>(Common::SCREEN_WIDTH), static_cast<float>(Common::SCREEN_HEIGHT),
                                  Common::TextureID::TEX_NONE, 0.0f, 0, 0, 0, 60});
    }

    if (m_gameState == Common::GameState::RUNNING || m_gameState == Common::GameState::PAUSED) {
        m_entityManager.render(frameCommands);
        if (m_levelEditor.isActive())
            m_levelEditor.render(frameCommands, m_textCommands);
        else
            m_progressIndicator.render(frameCommands);
    }

    if (m_isTransitioning) {
        frameCommands.push_back({0, 0, static_cast<float>(Common::SCREEN_WIDTH), static_cast<float>(Common::SCREEN_HEIGHT),
                                  Common::TextureID::TEX_NONE, 0.0f, 0, 0, 0, static_cast<unsigned char>(m_transitionAlpha)});
    }

    m_renderer.drawCommands(frameCommands, m_camera.getOffsetX());

    renderWelcomeScreen();
    renderMainMenuText();
    renderStatsScreenPanel();
    renderPausedText();
    renderHUDText(levelName);

    m_renderer.drawUI(m_uiCommands);

    for (const auto &tc : m_textCommands) {
        m_textRenderer.setActiveFont(tc.font);
        SDL_Color color = {tc.colorR, tc.colorG, tc.colorB, tc.colorA};
        auto tex = m_textRenderer.renderText(m_renderer.get(), tc.text, color);
        if (tex) {
            float tw, th;
            SDL_GetTextureSize(tex.get(), &tw, &th);
            float dx = tc.rightAligned ? Common::SCREEN_WIDTH - tw - tc.x : tc.centered ? tc.x - tw / 2.0f : tc.x;
            m_renderer.drawTexture(tex.get(), dx, tc.y, tw, th);
            m_frameTextures.push_back(std::move(tex));
        }
    }

    renderDeathScreen();
    renderLevelCompleteOverlay();

    m_renderer.endFrame();
}

void Game::renderFrameText(const std::string &font, const std::string &text,
                            float x, float y, SDL_Color color) {
    m_textRenderer.setActiveFont(font);
    auto tex = m_textRenderer.renderText(m_renderer.get(), text, color);
    if (tex) {
        float tw, th;
        SDL_GetTextureSize(tex.get(), &tw, &th);
        m_renderer.drawTexture(tex.get(), x, y, tw, th);
        m_frameTextures.push_back(std::move(tex));
    }
}

void Game::renderFrameCentered(const std::string &font, const std::string &text,
                                float cx, float cy, SDL_Color color) {
    m_textRenderer.setActiveFont(font);
    float tw, th;
    m_textRenderer.measureText(text, tw, th);
    auto tex = m_textRenderer.renderText(m_renderer.get(), text, color);
    if (tex) {
        m_renderer.drawTexture(tex.get(), cx - tw / 2.0f, cy, tw, th);
        m_frameTextures.push_back(std::move(tex));
    }
}

void Game::renderFrameTextWithShadow(const std::string &font, const std::string &text,
                                      float x, float y, SDL_Color color) {
    m_textRenderer.setActiveFont(font);
    float tw, th;
    m_textRenderer.measureText(text, tw, th);
    float shadowOffset = 2.0f;
    m_textCommands.push_back({text, x + shadowOffset, y + shadowOffset,
                              0, 0, 0, 180, false, font});
    m_textCommands.push_back({text, x, y, color.r, color.g, color.b, color.a, false, font});
}

std::string Game::extractLevelName(const std::string &levelPath) const {
    std::string name = levelPath;
    auto slashPos = name.rfind('/');
    if (slashPos != std::string::npos) name = name.substr(slashPos + 1);
    auto dotPos = name.rfind(".level");
    if (dotPos != std::string::npos) name = name.substr(0, dotPos);
    return name;
}

void Game::renderWelcomeScreen() {
    if (m_gameState != Common::GameState::WELCOME) return;

    renderFrameCentered("title", "WELCOME TO 2D PLATFORMER",
                   Common::SCREEN_WIDTH / 2.0f, Common::SCREEN_HEIGHT / 2.0f - 40.0f,
                   Common::Colors::TEXT_PRIMARY);

    std::string userName = m_currentUser ? m_currentUser->username : "Guest";
    renderFrameCentered("ui", userName,
                   Common::SCREEN_WIDTH / 2.0f, Common::SCREEN_HEIGHT / 2.0f + 10.0f,
                   Common::Colors::TEXT_ACCENT);
}

void Game::renderMainMenuText() {
    if (m_gameState != Common::GameState::MAIN_MENU) return;

    renderFrameCentered("title", "2D PLATFORMER",
                   Common::SCREEN_WIDTH / 2.0f, 80.0f,
                   Common::Colors::TEXT_PRIMARY);

    std::string userName = m_currentUser ? m_currentUser->username : "Guest";
    float tw, th;
    m_textRenderer.setActiveFont("ui");
    m_textRenderer.measureText(userName, tw, th);
    renderFrameText("ui", userName,
               Common::SCREEN_WIDTH - 15.0f - tw, 15.0f,
               Common::Colors::TEXT_ACCENT);
}

void Game::renderStatsScreenPanel() {
    if (m_gameState != Common::GameState::STATS_SCREEN) return;

    m_renderer.drawOverlay(180.0f);

    float px = (Common::SCREEN_WIDTH - Common::PAUSE_PANEL_WIDTH) / 2.0f;
    float py = (Common::SCREEN_HEIGHT - Common::PAUSE_PANEL_HEIGHT) / 2.0f;

    m_uiCommands.push_back({px - 3.0f, py - 3.0f,
                            Common::PAUSE_PANEL_WIDTH + 6.0f,
                            Common::PAUSE_PANEL_HEIGHT + 6.0f,
                            Common::TextureID::TEX_NONE, 0.0f,
                            Common::Colors::PANEL_BORDER.r,
                            Common::Colors::PANEL_BORDER.g,
                            Common::Colors::PANEL_BORDER.b,
                            Common::Colors::PANEL_BORDER.a});
    m_uiCommands.push_back({px, py,
                            Common::PAUSE_PANEL_WIDTH, Common::PAUSE_PANEL_HEIGHT,
                            Common::TextureID::TEX_NONE, 0.0f,
                            Common::Colors::PANEL_BG.r, Common::Colors::PANEL_BG.g,
                            Common::Colors::PANEL_BG.b, Common::Colors::PANEL_BG.a});

    float bx = px + (Common::PAUSE_PANEL_WIDTH - Common::MENU_BUTTON_WIDTH) / 2.0f;
    float by = py + Common::PAUSE_PANEL_HEIGHT - Common::MENU_BUTTON_HEIGHT - 20;
    int hovered = m_menuSystem.getHoveredButton();
    SDL_Color btnFill = (hovered == 4) ? Common::Colors::BTN_STATS_HOVER : Common::Colors::BTN_STATS;
    m_uiCommands.push_back({bx - 2.0f, by - 2.0f,
                            Common::MENU_BUTTON_WIDTH + 4.0f,
                            Common::MENU_BUTTON_HEIGHT + 4.0f,
                            Common::TextureID::TEX_NONE, 0.0f,
                            50, 205, 50, 255});
    m_uiCommands.push_back({bx, by, Common::MENU_BUTTON_WIDTH, Common::MENU_BUTTON_HEIGHT,
                            Common::TextureID::TEX_NONE, 0.0f,
                            btnFill.r, btnFill.g, btnFill.b, btnFill.a});

    float tx = px + 25.0f;
    float ty = py + 20.0f;
    float lh = 30.0f;

    m_textCommands.push_back({"Player:  " + (m_currentUser ? m_currentUser->username : "Guest"),
                              tx, ty, 0, 255, 255, 255});
    m_textCommands.push_back({"Total Deaths:  " + std::to_string(m_statsManager.getTotalDeaths()),
                              tx, ty + lh, 255, 255, 255, 255});
    m_textCommands.push_back({"Total Time:    " + Gameplay::StatsManager::formatTime(m_statsManager.getTotalPlaytime()),
                              tx, ty + lh * 2, 255, 255, 255, 255});
    m_textCommands.push_back({"Levels Done:   " + std::to_string(m_statsManager.getLevelsCompleted()),
                              tx, ty + lh * 3, 255, 255, 255, 255});
    m_textCommands.push_back({"Back", bx + Common::MENU_BUTTON_WIDTH / 2.0f, by + 32, 255, 255, 255, 255, true});
    m_textCommands.push_back({"Press ESC", tx, ty + lh * 4 + 10, 136, 136, 136, 255});
}

void Game::renderPausedText() {
    if (m_gameState != Common::GameState::PAUSED || m_levelEditor.isActive()) return;

    float px = (Common::SCREEN_WIDTH - Common::PAUSE_PANEL_WIDTH) / 2.0f;
    float py = (Common::SCREEN_HEIGHT - Common::PAUSE_PANEL_HEIGHT) / 2.0f;
    float tx = px + 20.0f;
    float ty = py + 10.0f;
    float lh = 26.0f;

    m_textCommands.push_back({"Total Deaths:  " + std::to_string(m_statsManager.getTotalDeaths()),
                              tx, ty, 200, 200, 200, 255});
    m_textCommands.push_back({"Total Time:    " + Gameplay::StatsManager::formatTime(m_statsManager.getTotalPlaytime()),
                              tx, ty + lh, 200, 200, 200, 255});
    m_textCommands.push_back({"Levels Done:   " + std::to_string(m_statsManager.getLevelsCompleted()),
                              tx, ty + lh * 2, 200, 200, 200, 255});
}

void Game::renderHUDText(const std::string &levelName) {
    if (m_gameState != Common::GameState::RUNNING || m_levelEditor.isActive()) return;

    std::string timeStr = Gameplay::StatsManager::formatTime(m_statsManager.getCurrentLevelTime());
    renderFrameTextWithShadow("hud-timer", timeStr, Common::HUD_MARGIN, Common::HUD_MARGIN,
                         Common::Colors::TEXT_PRIMARY);

    std::string deathsStr = "Deaths: " + std::to_string(m_statsManager.getCurrentDeaths());
    renderFrameTextWithShadow("hud-label", levelName, Common::HUD_MARGIN,
                         Common::SCREEN_HEIGHT - Common::HUD_MARGIN - Common::HUD_LINE_HEIGHT,
                         Common::Colors::TEXT_ACCENT);

    float lvlW, lvlH;
    m_textRenderer.setActiveFont("hud-label");
    m_textRenderer.measureText(levelName, lvlW, lvlH);
    renderFrameTextWithShadow("hud-label", deathsStr,
                         Common::HUD_MARGIN + lvlW + 20.0f,
                         Common::SCREEN_HEIGHT - Common::HUD_MARGIN - Common::HUD_LINE_HEIGHT,
                         Common::Colors::TEXT_DEATHS);
}

void Game::renderDeathScreen() {
    if (!m_isTransitioning || !m_isDeathRespawn) return;

    SDL_Texture *skullTex = m_renderer.getTexture(Common::TextureID::TEX_SKULL);
    if (skullTex) {
        float skullSize = 128.0f;
        float sx = (Common::SCREEN_WIDTH - skullSize) / 2.0f;
        float sy = (Common::SCREEN_HEIGHT - skullSize) / 2.0f - 40.0f;
        SDL_FRect dest = {sx, sy, skullSize, skullSize};

        SDL_SetTextureBlendMode(skullTex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(skullTex, static_cast<Uint8>(m_transitionAlpha));
        SDL_RenderTexture(m_renderer.get(), skullTex, NULL, &dest);
        SDL_SetTextureAlphaMod(skullTex, 255);
        SDL_SetTextureBlendMode(skullTex, SDL_BLENDMODE_NONE);
    }

    std::string dt = "YOU DIED";
    m_textRenderer.setActiveFont("title");
    float tw, th;
    m_textRenderer.measureText(dt, tw, th);
    auto shadowTex = m_textRenderer.renderText(m_renderer.get(), dt, {0, 0, 0, static_cast<Uint8>(m_transitionAlpha)});
    if (shadowTex) {
        m_renderer.drawTexture(shadowTex.get(), (Common::SCREEN_WIDTH - tw) / 2.0f + 2.0f,
            (Common::SCREEN_HEIGHT - 128) / 2.0f - 40.0f + 128 + 20 + 2.0f, tw, th);
        m_frameTextures.push_back(std::move(shadowTex));
    }
    auto tex = m_textRenderer.renderText(m_renderer.get(), dt,
        {255, 140, 0, static_cast<Uint8>(m_transitionAlpha)});
    if (tex) {
        m_renderer.drawTexture(tex.get(), (Common::SCREEN_WIDTH - tw) / 2.0f,
            (Common::SCREEN_HEIGHT - 128) / 2.0f - 40.0f + 128 + 20, tw, th);
        m_frameTextures.push_back(std::move(tex));
    }
}

void Game::renderLevelCompleteOverlay() {
    if (!(m_isTransitioning && m_showLevelComplete)
        && m_gameState != Common::GameState::LEVEL_COMPLETE) return;

    if (m_gameState == Common::GameState::LEVEL_COMPLETE)
        m_renderer.drawOverlay(255.0f);

    std::string lvlPath = m_statsManager.getLastCompletedLevel();
    std::string lvlName = extractLevelName(lvlPath);

    float cx = Common::SCREEN_WIDTH / 2.0f;
    float titleSpacing = 60.0f;
    float statSpacing = 30.0f;
    int numStats = (m_gameState == Common::GameState::LEVEL_COMPLETE) ? 4 : 3;
    float totalBlockHeight = titleSpacing + numStats * statSpacing;
    float cy = (Common::SCREEN_HEIGHT - totalBlockHeight) / 2.0f;

    auto renderLine = [&](const std::string &font, const std::string &text, float y, SDL_Color color) {
        m_textRenderer.setActiveFont(font);
        float tw2, th2;
        m_textRenderer.measureText(text, tw2, th2);
        auto tex2 = m_textRenderer.renderText(m_renderer.get(), text, color);
        if (tex2) {
            m_renderer.drawTexture(tex2.get(), cx - tw2 / 2.0f, y, tw2, th2);
            m_frameTextures.push_back(std::move(tex2));
        }
    };

    renderLine("title", lvlName + " Complete!", cy, Common::Colors::BTN_PLAY);
    renderLine("ui", "Time:  " + Gameplay::StatsManager::formatTime(m_statsManager.getLastCompletedTime()),
       cy + titleSpacing, Common::Colors::TEXT_PRIMARY);
    renderLine("ui", "Deaths: " + std::to_string(m_statsManager.getLastCompletedDeaths()),
       cy + titleSpacing + statSpacing, Common::Colors::TEXT_PRIMARY);

    if (m_statsManager.getLastCompletedNewBest())
        renderLine("ui", "Best:  " + Gameplay::StatsManager::formatTime(m_statsManager.getBestTime(lvlPath)) + " (NEW!)",
           cy + titleSpacing + statSpacing * 2, Common::Colors::BTN_PLAY);
    else
        renderLine("ui", "Best:  " + Gameplay::StatsManager::formatTime(m_statsManager.getBestTime(lvlPath)),
           cy + titleSpacing + statSpacing * 2, Common::Colors::TEXT_PRIMARY);

    if (m_gameState == Common::GameState::LEVEL_COMPLETE)
        renderLine("ui", "Press any key to continue",
           cy + titleSpacing + statSpacing * 3 + 10, Common::Colors::TEXT_DIM);
}

void Game::syncUserStats() {
    /// Copies all StatsManager data into the AuthService user record and
    /// persists to disk via saveUserStatsToFile. No-op if no user or auth service.
    if (!m_currentUser || !m_authService) return;
    auto *stats = m_authService->getUserStats(m_currentUser->username);
    if (stats) {
        stats->totalDeaths = m_statsManager.getTotalDeaths();
        stats->totalPlaytime = m_statsManager.getTotalPlaytime();
        stats->levelsCompleted = m_statsManager.getLevelsCompleted();
        stats->completedLevels = m_statsManager.getCompletedLevels();
        stats->bestTimes = m_statsManager.getBestTimes();
    }
    m_authService->saveUserStatsToFile(m_currentUser->username);
}

bool Game::loadAndSetupLevel(const std::string &levelPath) {
    /// Clears all existing entities, loads a new level from disk, recalculates
    /// camera bounds, updates m_currentLevel. Sets m_gameState to EXITING on
    /// failure so the caller can bail out immediately.
    /// @param levelPath  Path to the .level file.
    /// @return true on success, false if loading failed.
    m_entityManager.clear();
    if (!Gameplay::LevelLoader::loadLevel(m_entityManager, levelPath)) {
        m_gameState = Common::GameState::EXITING;
        return false;
    }
    m_camera.setBounds(0.0f, std::max(0.0f, m_entityManager.levelConfig.levelWidth - Common::VIEWPORT_WIDTH));
    m_currentLevel = levelPath;
    return true;
}

bool Game::runOneFrame() {
    /// Executes a single frame of the game loop: accumulator-based fixed
    /// timestep update, input polling, render, and FPS calculation.
    /// @return false if the game loop should stop (state == EXITING).
    if (!isRunning()) return false;

    Uint64 now = SDL_GetTicks();
    float frameTime = (now - m_lastTime) / 1000.0f;
    m_lastTime = now;

    if (frameTime > Common::MAX_FRAME_TIME)
        frameTime = Common::MAX_FRAME_TIME;
    m_accumulator += frameTime;

    while (m_accumulator >= Common::TIME_STEP) {
        Common::InputState input = m_inputManager.update(m_renderer.get());
        handleInput(input);
        if (!isRunning()) break;
        update(Common::TIME_STEP, input);
        m_accumulator -= Common::TIME_STEP;
    }

    render();

    m_fpsFrames++;
    float elapsed = (SDL_GetTicks() - m_fpsTimer) / 1000.0f;
    if (elapsed >= Common::FPS_UPDATE_INTERVAL) {
        int fps = static_cast<int>(m_fpsFrames / elapsed);
        std::string title = std::string(Common::WINDOW_TITLE)
                          + " - " + std::to_string(fps) + " FPS";
        m_window.setTitle(title);
        m_fpsFrames = 0;
        m_fpsTimer = SDL_GetTicks();
    }

    return isRunning();
}

void Game::onLoopEnd() {
    /// Persists player position and stats when the game loop exits.
    if (m_currentUser && m_authService) {
        auto player = m_entityManager.getPlayer();
        if (player) {
            m_authService->updateProgress(m_currentUser->username, m_currentLevel,
                                          player->transform.x, player->transform.y);
        }
        syncUserStats();
        m_authService->saveUsersToFile();
    }
}

void Game::run() {
    /// The main game loop: initialises timing state, then either runs a
    /// blocking while loop (native) or sets up an Emscripten rAF-based
    /// main loop callback (WASM). On exit, persists player progress.
    m_lastTime = SDL_GetTicks();
    m_fpsTimer = m_lastTime;
    m_fpsFrames = 0;
    m_accumulator = 0.0f;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(
        [](void *arg) {
            Game *g = static_cast<Game *>(arg);
            if (!g->runOneFrame()) {
                g->onLoopEnd();
                emscripten_cancel_main_loop();
            }
        },
        this, 0, 1
    );
#else
    while (runOneFrame()) {}
    onLoopEnd();
#endif
}

} // namespace Engine
