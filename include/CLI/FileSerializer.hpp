/// @file FileSerializer.hpp
/// File I/O utility: encrypted storage for user accounts and stats,
/// and plaintext serialisation of level data. All methods are static.

#pragma once

#include "CLI/AuthService.hpp"
#include "Gameplay/EntityManager.hpp"
#include <string>
#include <vector>

namespace CLI {

/// File I/O utility: encrypted storage for user accounts and stats,
/// and plaintext serialisation of level data.
/// All methods are static.
class FileSerializer {
public:
  // --- Password handling ---
  /// Custom hash: XOR + Caesar + mixing rounds over password + salt.
  static std::string hashPassword(const std::string &password);
  /// Constant-time comparison to prevent timing attacks.
  static bool verifyPassword(const std::string &password,
                             const std::string &hash);

  // --- Encryption primitives ---
  /// Caesar cipher: rotates letters A-Z,a-z and digits 0-9.
  static std::string caesarCipher(const std::string &input, int shift = 3);

  /// XOR obfuscation with a repeating key (XOR_KEY).
  /// @param data  Input string to transform.
  /// @return XOR-obfuscated copy of the input.
  static std::string xorTransform(const std::string &data);

  // --- Path helpers ---
  /// Derives the per-user stats file path from username (Caesar-ciphered).
  static std::string statsFilename(const std::string &username);

  // --- File I/O ---
  /// Writes XOR-encrypted content to a binary file.
  static bool saveEncryptedFile(const std::string &filePath,
                                const std::string &content);
  /// Reads and XOR-decrypts a binary file.
  static bool loadEncryptedFile(const std::string &filePath,
                                std::string &content);

  /// Serialises the user list and writes it to an encrypted file.
  static bool saveUsers(const std::vector<UserData> &users,
                        const std::string &filePath);
  /// Reads and parses the encrypted user database.
  static bool loadUsers(const std::string &filePath,
                        std::vector<UserData> &users);

  /// Saves a single user's stats to their encrypted stats file.
  static bool saveUserStats(const std::string &username,
                            const UserStats &stats);

  /// Loads a single user's stats from their encrypted stats file.
  static bool loadUserStats(const std::string &username, UserStats &stats);

  /// Serialises the entire EntityManager state into a plaintext .level file.
  static bool saveLevel(const Gameplay::EntityManager &entityManager,
                        const std::string &filePath);

  // --- String utilities (delegate to Common::Util) ---

private:
  static constexpr const char *SALT =
      "Cpp2DPlatformerGameSalt!"; ///< Salt for password hashing.
  static constexpr const char *XOR_KEY =
      "2DPlatformerXORK3y!";             ///< Key for XOR obfuscation.
  static constexpr int CAESAR_SHIFT = 3; ///< Shift for Caesar cipher.
};

} // namespace CLI
