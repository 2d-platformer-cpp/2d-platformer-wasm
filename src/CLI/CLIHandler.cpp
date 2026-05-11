/// @file CLIHandler.cpp
/// Implementation of the CLIHandler class: terminal I/O for authentication
/// flow, admin dashboard, password masking, line input, and validated integer
/// input. All methods operate purely on stdin/stdout.

#include "CLI/CLIHandler.hpp"
#include <iostream>
#ifndef __EMSCRIPTEN__
#include <termios.h>
#include <unistd.h>
#endif
#include <filesystem>
#include <limits>

namespace CLI {

std::string CLIHandler::getPasswordPrompt(const std::string &prompt) {
    /// Reads a password from stdin with asterisk masking and no echo.
    /// Disables terminal echo and canonical mode, reads characters one at
    /// a time, displays '*' for each, handles backspace (127 or 8) for
    /// deletion, and restores terminal settings on exit.
    /// @param prompt  The prompt text to display.
    /// @return The entered password string.
    std::cout << prompt << std::flush;
    std::string password;
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    struct TerminalGuard {
        struct termios saved;
        ~TerminalGuard() { tcsetattr(STDIN_FILENO, TCSANOW, &saved); }
    } guard{oldt};

    char ch = 0;
    while (read(STDIN_FILENO, &ch, 1) > 0 && ch != '\n' && ch != '\r') {
        if (ch == 127 || ch == 8) {
            if (!password.empty()) { password.pop_back(); std::cout << "\b \b" << std::flush; }
        } else if (ch >= 32 && ch <= 126) {
            password += ch;
            std::cout << '*' << std::flush;
        }
    }
    std::cout << std::endl;
    return password;
}

std::string CLIHandler::getLine(const std::string &prompt) {
    /// Reads a single line of text from stdin.
    /// @param prompt  The prompt text to display.
    /// @return The entered line (without trailing newline).
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

int CLIHandler::getValidInt(int min, int max, const std::string &prompt) {
    /// Reads an integer from stdin within a given inclusive range.
    /// Loops until valid input is provided, clearing the fail state and
    /// ignoring the rest of the line on each invalid attempt.
    /// @param min, max  Valid range (inclusive).
    /// @param prompt    The prompt text.
    /// @return The validated integer.
    int value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        if (std::cin.fail() || value < min || value > max) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Enter " << min << "-" << max << ": ";
        } else {
            std::cin.ignore(10000, '\n');
            return value;
        }
    }
}

std::optional<User> CLIHandler::runAuthFlow(AuthService &authService) {
    /// Presents a login/register loop: displays a menu (1=Login, 2=Register),
    /// prompts for username and password (masked), authenticates against or
    /// registers with the AuthService, and returns the authenticated User on
    /// success. Loops until successful authentication or registration.
    /// @param authService  The auth service to authenticate against.
    /// @return An authenticated User, or nullopt if the user chose to exit.
    std::cout << "\n=== 2D Platformer ===\n" << std::endl;

    while (true) {
        std::cout << "1. Login\n2. Register\nChoice: ";
        int choice;
        std::cin >> choice;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input." << std::endl;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice != 1 && choice != 2) {
            std::cout << "Invalid choice." << std::endl;
            continue;
        }

        std::string username = getLine("Username: ");
        std::string password = getPasswordPrompt("Password: ");

        if (choice == 1) {
            auto user = authService.authenticate(username, password);
            if (user) {
                std::cout << "Welcome, " << user->username << "!" << std::endl;
                return user;
            }
            std::cout << "Invalid credentials." << std::endl;
        } else {
            if (authService.registerUser(username, password)) {
                std::cout << "Registered successfully!" << std::endl;
                auto user = authService.authenticate(username, password);
                if (user) {
                    std::cout << "Welcome, " << user->username << "!" << std::endl;
                    return user;
                }
            }
            std::cout << "Username already exists." << std::endl;
        }
    }
}

CLIHandler::AdminDashboardResult CLIHandler::runAdminDashboard(const User &user) {
    /// Presents the admin level-selection and editor-launch menu. Non-admin
    /// users bypass the menu and immediately return with their current level.
    /// Admin menu options: show levels (list .level files), create level
    /// (enter editor with empty level), play game (enter level name), log out.
    /// @param user  The authenticated admin user.
    /// @return The admin's chosen action (which level, enter editor, etc.).
    AdminDashboardResult result;

    if (user.role != UserRole::Admin) {
        result.levelToLoad = user.currentLevel;
        return result;
    }

    while (true) {
        std::cout << "\n=== Admin Dashboard ===\n";
        std::cout << "1. Show levels\n2. Create level\n3. Play game\n4. Log out\nChoice: ";
        int choice;
        std::cin >> choice;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input." << std::endl;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 1) {
            std::cout << "\nLevels:\n";
            std::string dir = "assets/levels";
            if (std::filesystem::exists(dir)) {
                for (const auto &entry : std::filesystem::directory_iterator(dir)) {
                    std::string name = entry.path().filename().string();
                    if (name.find(".level") != std::string::npos)
                        std::cout << "  - " << name << std::endl;
                }
            }
        } else if (choice == 2) {
            result.createNewLevel = true;
            result.enterEditor = true;
            break;
        } else if (choice == 3) {
            std::string levelName = getLine("Enter level name: ");
            if (!levelName.empty()) {
                result.levelToLoad = levelName;
                if (result.levelToLoad.find(".level") == std::string::npos)
                    result.levelToLoad += ".level";
            }
            break;
        } else if (choice == 4) {
            result.shouldExitApp = true;
            break;
        }
    }
    return result;
}

} // namespace CLI
