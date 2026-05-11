/// @file AuthService.hpp
/// Manages user accounts: registration, authentication, progress tracking,
/// and statistics persistence. Uses FileSerializer for encrypted file I/O.

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace CLI {

/// User role/permission level.
enum class UserRole { Player, Admin };

/// Aggregate statistics tracked per user.
struct UserStats {
    int totalDeaths = 0;                                    ///< Cumulative death count.
    float totalPlaytime = 0.0f;                             ///< Cumulative playtime in seconds.
    int levelsCompleted = 0;                                ///< Number of distinct levels completed.
    std::vector<std::string> completedLevels;               ///< Paths of completed levels.
    std::unordered_map<std::string, float> bestTimes;       ///< Best completion time per level (levelPath → seconds).
};

/// Public-safe user info (excludes password hash).
struct User {
    std::string username;            ///< Display name.
    UserRole role = UserRole::Player;///< Permission level.
    std::string currentLevel;        ///< Last played level path.
    float posX = 0.0f;               ///< Last known player X position.
    float posY = 0.0f;               ///< Last known player Y position.
};

/// Full user record including hashed password (for internal use).
struct UserData {
    std::string username;            ///< Unique display name.
    std::string password;            ///< Hashed password.
    UserRole role = UserRole::Player;///< Permission level.
    std::string currentLevel;        ///< Last played level path.
    float posX = 0.0f;               ///< Last known player X.
    float posY = 0.0f;               ///< Last known player Y.
    UserStats stats;                 ///< Cumulative player statistics.
};

/// Manages user accounts: registration, authentication, progress tracking,
/// and statistics persistence. Uses FileSerializer for encrypted file I/O.
class AuthService {
public:
    /// Constructs the AuthService and loads the user database from the encrypted users file.
    AuthService();

    /// Attempts to log in with the given credentials.
    /// @param username  The user's display name.
    /// @param password  The plaintext password (will be hashed for comparison).
    /// @return A User object on success, nullopt on failure.
    std::optional<User> authenticate(const std::string &username, const std::string &password);

    /// Creates a new user account.
    /// @param username  Desired display name (must not already exist).
    /// @param password  Plaintext password (will be hashed before storage).
    /// @param role      User role (defaults to Player).
    /// @return true if the account was created successfully.
    bool registerUser(const std::string &username, const std::string &password, UserRole role = UserRole::Player);

    /// Updates the user's last-played position and level for resume-on-login.
    /// @param username  The user to update.
    /// @param level     Current level path.
    /// @param x, y      Player world position.
    void updateProgress(const std::string &username, const std::string &level, float x, float y);

    /// Writes a single user's stats to their encrypted stats file.
    /// @param username  The user whose stats to save.
    void saveUserStatsToFile(const std::string &username);

    /// Reads a single user's stats from their encrypted stats file into memory.
    /// @param username  The user whose stats to load.
    void loadUserStatsFromFile(const std::string &username);

    /// Writes the full user database to the encrypted users file.
    void saveUsersToFile();

    /// Returns a mutable pointer to a user's statistics.
    /// @param username  The user to look up.
    /// @return Pointer to UserStats, or nullptr if user not found.
    UserStats *getUserStats(const std::string &username);

private:
    std::vector<UserData> m_users;  ///< In-memory user database.
    void loadUsers();               ///< Load users from encrypted file on construction.
    void saveUsers();               ///< (Unused internal; saveUsersToFile is the public wrapper.)
};

} // namespace CLI
