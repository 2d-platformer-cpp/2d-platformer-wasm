/// @file AuthService.cpp
/// Implementation of the AuthService class: user database loading/saving,
/// registration, authentication, progress tracking, and user-stats file I/O.

#include "CLI/AuthService.hpp"
#include "CLI/FileSerializer.hpp"
#include "Common/Constants.hpp"

namespace CLI {

AuthService::AuthService() {
    /// Constructs the AuthService and loads the user database from the
    /// encrypted users file. If the database is empty (first run), creates
    /// default admin (admin/admin123) and player (player/player123) accounts.
    loadUsers();
    if (m_users.empty()) {
        UserData admin;
        admin.username = "admin";
        admin.password = FileSerializer::hashPassword("admin123");
        admin.role = UserRole::Admin;
        admin.currentLevel = "assets/levels/level1.level";
        admin.stats = {};
        m_users.push_back(admin);

        UserData player;
        player.username = "player";
        player.password = FileSerializer::hashPassword("player123");
        player.role = UserRole::Player;
        player.currentLevel = "assets/levels/level1.level";
        player.stats = {};
        m_users.push_back(player);

        saveUsers();
        FileSerializer::saveUserStats("admin", {});
        FileSerializer::saveUserStats("player", {});
    }
}

std::optional<User> AuthService::authenticate(const std::string &username, const std::string &password) {
    /// Searches the user database for a matching username and verifies the
    /// password hash. Returns a public-safe User object on success.
    /// @param username  The user's display name.
    /// @param password  Plaintext password to verify.
    /// @return A User object on success, nullopt on failure.
    for (const auto &u : m_users) {
        if (u.username == username && FileSerializer::verifyPassword(password, u.password)) {
            return User{u.username, u.role, u.currentLevel, u.posX, u.posY};
        }
    }
    return std::nullopt;
}

bool AuthService::registerUser(const std::string &username, const std::string &password, UserRole role) {
    /// Creates a new user account if the username is not taken. Hashes the
    /// password, persists the updated database and creates an empty stats file.
    /// @param username  Desired display name (must not already exist).
    /// @param password  Plaintext password (will be hashed before storage).
    /// @param role      User role (defaults to Player).
    /// @return true if the account was created successfully.
    for (const auto &u : m_users) {
        if (u.username == username) return false;
    }
    UserData newUser;
    newUser.username = username;
    newUser.password = FileSerializer::hashPassword(password);
    newUser.role = role;
    newUser.currentLevel = "assets/levels/level1.level";
    newUser.stats = {};
    m_users.push_back(newUser);
    saveUsers();
    FileSerializer::saveUserStats(username, {});
    return true;
}

void AuthService::updateProgress(const std::string &username, const std::string &level, float x, float y) {
    /// Updates the user's last-played level and world position (in memory).
    /// Persist is done separately via saveUsersToFile() or saveUsers().
    /// @param username  The user to update.
    /// @param level     Current level path.
    /// @param x, y      Player world position.
    for (auto &u : m_users) {
        if (u.username == username) {
            u.currentLevel = level;
            u.posX = x;
            u.posY = y;
            return;
        }
    }
}

void AuthService::saveUserStatsToFile(const std::string &username) {
    /// Writes a single user's statistics to their encrypted per-user stats file.
    /// @param username  The user whose stats to save.
    for (const auto &u : m_users) {
        if (u.username == username) {
            FileSerializer::saveUserStats(username, u.stats);
            return;
        }
    }
}

void AuthService::loadUserStatsFromFile(const std::string &username) {
    /// Reads a single user's statistics from their encrypted per-user stats
    /// file into the in-memory database.
    /// @param username  The user whose stats to load.
    for (auto &u : m_users) {
        if (u.username == username) {
            UserStats stats;
            if (FileSerializer::loadUserStats(username, stats))
                u.stats = stats;
            return;
        }
    }
}

UserStats *AuthService::getUserStats(const std::string &username) {
    /// Returns a mutable pointer to a user's statistics for direct inspection
    /// or modification. Returns nullptr if the user is not found.
    /// @param username  The user to look up.
    /// @return Pointer to UserStats, or nullptr.
    for (auto &u : m_users) {
        if (u.username == username)
            return &u.stats;
    }
    return nullptr;
}

void AuthService::saveUsersToFile() {
    /// Public wrapper: writes the full user database to the encrypted users file.
    saveUsers();
}

void AuthService::loadUsers() {
    /// Loads the user database from the encrypted users file into memory.
    FileSerializer::loadUsers(Common::USERS_FILE_PATH, m_users);
}

void AuthService::saveUsers() {
    /// Serialises the in-memory user database to the encrypted users file.
    FileSerializer::saveUsers(m_users, Common::USERS_FILE_PATH);
}

} // namespace CLI
