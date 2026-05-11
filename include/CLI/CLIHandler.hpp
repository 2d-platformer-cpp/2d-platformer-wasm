/// @file CLIHandler.hpp
/// Terminal I/O for authentication flow, admin dashboard, and input prompts.
/// All methods are static; this class operates purely on stdin/stdout.

#pragma once

#include "CLI/AuthService.hpp"
#include "Common/Constants.hpp"
#include <optional>
#include <string>

namespace CLI {

/// Terminal I/O for authentication flow, admin dashboard, and input prompts.
/// All methods are static; this class operates purely on stdin/stdout.
class CLIHandler {
public:
  /// Result from the admin dashboard: what action the admin chose.
  struct AdminDashboardResult {
    bool enterEditor = false;    ///< Launch in-editor mode.
    bool createNewLevel = false; ///< Create a new empty level.
    float newLevelWidth =
        Common::DEFAULT_LEVEL_WIDTH; ///< Width for new levels.
    std::string levelToLoad = "";    ///< Path of the level to load.
    bool shouldExitApp = false;      ///< Exit the application entirely.
  };

  /// Presents the login/register prompt loop and returns an authenticated user.
  /// @param authService  The auth service to authenticate against.
  /// @return An authenticated User, or nullopt if the user chose to exit.
  static std::optional<User> runAuthFlow(AuthService &authService);

  /// Presents the admin level-selection and editor-launch menu.
  /// @param user  The authenticated admin user.
  /// @return The admin's chosen action.
  static AdminDashboardResult runAdminDashboard(const User &user);

  /// Reads a password from stdin with asterisk masking (no echo).
  /// @param prompt  The prompt text to display.
  /// @return The entered password string.
  static std::string getPasswordPrompt(const std::string &prompt);

  /// Reads a single line of text from stdin.
  /// @param prompt  The prompt text to display.
  /// @return The entered line.
  static std::string getLine(const std::string &prompt);

  /// Reads an integer from stdin within a given range.
  /// @param min, max  Valid range (inclusive).
  /// @param prompt    The prompt text.
  /// @return The validated integer.
  static int getValidInt(int min, int max, const std::string &prompt);
};

} // namespace CLI
