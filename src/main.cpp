/// @file main.cpp
/// Program entry point. In native builds, authenticates the user via CLI,
/// then launches the game loop (Player) or admin dashboard (Admin).
/// In WASM builds, skips CLI auth entirely and starts the game directly.

#include <iostream>
#include "Common/Constants.hpp"
#include "Engine/Game.hpp"

#ifndef __EMSCRIPTEN__
#include "CLI/AuthService.hpp"
#include "CLI/CLIHandler.hpp"
#include "CLI/FileSerializer.hpp"
#endif

int main() {
#ifdef __EMSCRIPTEN__
    // WASM: skip CLI auth, start game directly with a default player
    std::optional<CLI::User> currentUser = CLI::User{
        "Player", CLI::UserRole::Player, "assets/levels/level1.level", 0.0f, 0.0f
    };
    Engine::Game game;
    game.initialize("assets/levels/level1.level", false,
                    Common::DEFAULT_LEVEL_WIDTH, currentUser, nullptr);
    game.run();
    return 0;
#else
    /// Entry point: creates the AuthService, runs the auth flow, and dispatches
    /// to either the player game loop or admin dashboard loop.
    /// Player path: initialises the Game with the user's last-played level,
    /// runs the game loop, and exits.
    /// Admin path: repeatedly shows the dashboard until the admin chooses to
    /// exit. Each iteration creates a fresh Game instance with optional editor
    /// mode. On editor save-and-exit (hasPendingSubmission), prompts for a
    /// level name and saves via FileSerializer::saveLevel.
    /// @return 0 on normal exit, 1 if no user authenticated.
    CLI::AuthService authService;
    auto currentUser = CLI::CLIHandler::runAuthFlow(authService);
    if (!currentUser) return 1;

    if (currentUser->role == CLI::UserRole::Player) {
        Engine::Game game;
        std::string startLevel = currentUser->currentLevel;
        if (startLevel.empty())
            startLevel = "assets/levels/level1.level";
        game.initialize(startLevel, false,
                        Common::DEFAULT_LEVEL_WIDTH, currentUser, &authService);
        game.run();
        return 0;
    }

    bool appRunning = true;
    while (appRunning) {
        auto adminResult = CLI::CLIHandler::runAdminDashboard(*currentUser);
        if (adminResult.shouldExitApp) break;

        {
            std::string levelName = adminResult.levelToLoad;
            size_t sep = levelName.find_last_of("/\\");
            if (sep != std::string::npos) levelName = levelName.substr(sep + 1);
            std::string levelPath = adminResult.createNewLevel ? "" :
                (levelName.empty() ? "assets/levels/level1.level" :
                 "assets/levels/" + levelName + (levelName.size() > 6 && levelName.substr(levelName.size() - 6) == ".level" ? "" : ".level"));

            Engine::Game game;
            game.initialize(levelPath, adminResult.enterEditor, adminResult.newLevelWidth, currentUser, &authService);
            game.run();

            if (game.hasPendingSubmission()) {
                std::cout << "\n=== Level Submission ===\n";
                std::cout << "Enter level name: ";
                std::string name;
                std::getline(std::cin >> std::ws, name);
                size_t sepPos = name.find_last_of("/\\");
                if (sepPos != std::string::npos) name = name.substr(sepPos + 1);
                if (name.empty() || name.find("..") != std::string::npos || name[0] == '.')
                    name = "untitled.level";
                if (name.find(".level") == std::string::npos) name += ".level";
                if (CLI::FileSerializer::saveLevel(game.getEntityManager(), "assets/levels/" + name))
                    std::cout << "Saved: assets/levels/" << name << std::endl;
                else
                    std::cout << "Save failed." << std::endl;
            }

            if (game.shouldExitApp())
                appRunning = false;
        }
    }
    return 0;
#endif
}
