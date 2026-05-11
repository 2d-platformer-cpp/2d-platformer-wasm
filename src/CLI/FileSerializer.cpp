/// @file FileSerializer.cpp
/// Implementation of the FileSerializer class: encrypted file I/O for user
/// accounts and stats, plaintext level serialisation, and password hashing
/// with Caesar/XOR/mixing-round obfuscation primitives.

#include "CLI/FileSerializer.hpp"
#include "Common/Constants.hpp"
#include "Common/Utilities.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <array>
#include <cstring>

using namespace Common::Util;

namespace CLI {

std::string FileSerializer::caesarCipher(const std::string &input, int shift) {
    /// Applies a Caesar cipher to letters A-Z, a-z and digits 0-9.
    /// Non-alphanumeric characters pass through unchanged. Shift wraps
    /// modulo 26 for letters and modulo 10 for digits.
    /// @param input  The string to transform.
    /// @param shift  Rotation amount (default 3).
    /// @return Caesar-shifted copy of the input.
    std::string result;
    result.reserve(input.size());
    shift = ((shift % 26) + 26) % 26;
    for (unsigned char c : input) {
        if (c >= 'a' && c <= 'z')
            result += static_cast<char>('a' + (c - 'a' + shift) % 26);
        else if (c >= 'A' && c <= 'Z')
            result += static_cast<char>('A' + (c - 'A' + shift) % 26);
        else if (c >= '0' && c <= '9')
            result += static_cast<char>('0' + (c - '0' + shift) % 10);
        else
            result += c;
    }
    return result;
}

std::string FileSerializer::xorTransform(const std::string &data) {
    /// Applies XOR obfuscation with the repeating XOR_KEY.
    /// Each byte of the input is XORed with the corresponding byte of the
    /// key (wrapping). The transform is symmetric (applying twice returns
    /// the original).
    /// @param data  Input string to transform.
    /// @return XOR-obfuscated copy of the input.
    std::string result = data;
    size_t keyLen = std::char_traits<char>::length(XOR_KEY);
    for (size_t i = 0; i < result.size(); i++)
        result[i] ^= XOR_KEY[i % keyLen];
    return result;
}

std::string FileSerializer::statsFilename(const std::string &username) {
    /// Derives the per-user encrypted stats file path by applying a Caesar
    /// cipher to the username and appending ".user" in the users directory.
    /// @param username  The user's display name.
    /// @return Full path like "assets/users/<ciphered_name>.user".
    return std::string(Common::USERS_DIR_PATH) + "/" + caesarCipher(username) + ".user";
}

bool FileSerializer::saveEncryptedFile(const std::string &filePath, const std::string &content) {
    /// XOR-encrypts the content and writes it to a binary file.
    /// @param filePath  Destination path.
    /// @param content   Plaintext content to encrypt and write.
    /// @return true on success, false if the file could not be opened or written.
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;
    std::string encrypted = xorTransform(content);
    file.write(encrypted.data(), encrypted.size());
    file.close();
    return file.good();
}

bool FileSerializer::loadEncryptedFile(const std::string &filePath, std::string &content) {
    /// Reads a binary file and XOR-decrypts its contents.
    /// @param filePath  Source path.
    /// @param content   [out] Decrypted content.
    /// @return true on success, false if the file could not be opened or read.
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string buffer(static_cast<size_t>(size), '\0');
    if (!file.read(buffer.data(), size)) { file.close(); return false; }
    file.close();
    content = xorTransform(buffer);
    return true;
}

bool FileSerializer::saveUserStats(const std::string &username, const UserStats &stats) {
    /// Serialises a UserStats struct to a pipe-delimited format and writes it
    /// to the user's encrypted stats file. Format:
    /// totalDeaths|totalPlaytime|levelsCompleted|completedLevels(comma-sep)|bestTimes(level=time,comma-sep)
    /// @param username  The user whose stats to save.
    /// @param stats     The statistics to serialise.
    /// @return true on success.
    std::ostringstream oss;
    oss << stats.totalDeaths << "|"
        << floatToString(stats.totalPlaytime) << "|"
        << stats.levelsCompleted << "|";
    for (size_t i = 0; i < stats.completedLevels.size(); i++) {
        if (i > 0) oss << ",";
        oss << stats.completedLevels[i];
    }
    oss << "|";
    {
        std::vector<std::pair<std::string, float>> sorted(begin(stats.bestTimes), end(stats.bestTimes));
        std::sort(sorted.begin(), sorted.end());
        bool first = true;
        for (const auto &[level, time] : sorted) {
            if (!first) oss << ",";
            oss << level << "=" << floatToString(time);
            first = false;
        }
    }
    oss << "\n";
    return saveEncryptedFile(statsFilename(username), oss.str());
}

bool FileSerializer::loadUserStats(const std::string &username, UserStats &stats) {
    /// Reads and parses the user's encrypted stats file back into a UserStats
    /// struct. Returns false if the file is missing, malformed, or empty.
    /// @param username  The user whose stats to load.
    /// @param stats     [out] Parsed statistics.
    /// @return true on success.
    std::string content;
    if (!loadEncryptedFile(statsFilename(username), content)) return false;
    auto parts = split(trim(content), '|');
    if (parts.size() < 3) return false;
    try {
        UserStats temp;
        temp.totalDeaths = std::stoi(parts[0]);
        temp.totalPlaytime = parseFloat(parts[1]);
        temp.levelsCompleted = std::stoi(parts[2]);

        temp.completedLevels.clear();
        if (parts.size() >= 4 && !parts[3].empty()) {
            auto levelParts = split(parts[3], ',');
            for (const auto &lp : levelParts) {
                std::string trimmed = trim(lp);
                if (!trimmed.empty())
                    temp.completedLevels.push_back(trimmed);
            }
        }

        temp.bestTimes.clear();
        if (parts.size() >= 5 && !parts[4].empty()) {
            auto entryParts = split(parts[4], ',');
            for (const auto &ep : entryParts) {
                auto eqPos = ep.find('=');
                if (eqPos != std::string::npos) {
                    std::string level = trim(ep.substr(0, eqPos));
                    float time = parseFloat(trim(ep.substr(eqPos + 1)));
                    if (!level.empty())
                        temp.bestTimes[level] = time;
                }
            }
        }

        stats = std::move(temp);
    } catch (...) { return false; }
    return true;
}

std::string FileSerializer::hashPassword(const std::string &password) {
    /// Custom hash function: XOR + mixing rounds over password + SALT.
    /// Combines the password with SALT, applies a series of XOR, shift, and
    /// add operations across a 32-byte hash buffer, then runs 3 mixing rounds.
    /// Output is a 64-character hex string.
    /// @param password  Plaintext password to hash.
    /// @return 64-character hex-encoded hash string.
    std::string combined = password + SALT;
    std::array<unsigned char, 32> hash = {0};

    for (size_t i = 0; i < combined.size(); i++) {
        unsigned char c = static_cast<unsigned char>(combined[i]);
        hash[i % 32] ^= c;
        hash[(i + 7) % 32] += c;
        hash[(i + 13) % 32] ^= (c << 2);
        hash[(i + 23) % 32] += (c >> 1);
    }

    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 32; i++) {
            hash[i] ^= hash[(i + 1) % 32];
            hash[i] += hash[(i + 5) % 32];
        }
    }

    std::ostringstream oss;
    for (auto b : hash)
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    return oss.str();
}

bool FileSerializer::verifyPassword(const std::string &password, const std::string &hash) {
    /// Constant-time comparison between a freshly-computed hash and the
    /// stored hash to prevent timing attacks. Uses XOR-accumulation rather
    /// than short-circuit comparison.
    /// @param password  Plaintext password to verify.
    /// @param hash      Stored hash to compare against.
    /// @return true if the password matches the hash.
    std::string inputHash = hashPassword(password);
    if (inputHash.length() != hash.length()) return false;
    unsigned char result = 0;
    for (size_t i = 0; i < hash.length(); i++)
        result |= static_cast<unsigned char>(inputHash[i]) ^ static_cast<unsigned char>(hash[i]);
    return result == 0;
}

bool FileSerializer::saveUsers(const std::vector<UserData> &users, const std::string &filePath) {
    /// Serialises the full user database to an encrypted file. Format:
    /// username|password_hash|role(0/1)|currentLevel|posX|posY|totalDeaths|totalPlaytime|levelsCompleted
    /// @param users    The user database to serialise.
    /// @param filePath  Destination path.
    /// @return true on success.
    std::ostringstream oss;

    oss << "# User Database - 2D Platformer Game\n";
    oss << "# Format: username|password_hash|role|currentLevel|posX|posY|totalDeaths|totalPlaytime|levelsCompleted\n";
    oss << "# Role: 0=Player, 1=Admin\n\n";

    for (const auto &user : users) {
        oss << user.username << "|"
            << user.password << "|"
            << static_cast<int>(user.role) << "|"
            << user.currentLevel << "|"
            << user.posX << "|"
            << user.posY << "|"
            << user.stats.totalDeaths << "|"
            << floatToString(user.stats.totalPlaytime) << "|"
            << user.stats.levelsCompleted << "\n";
    }

    return saveEncryptedFile(filePath, oss.str());
}

bool FileSerializer::loadUsers(const std::string &filePath, std::vector<UserData> &users) {
    /// Reads and parses the encrypted user database file back into a vector
    /// of UserData structs. Returns false if the file is missing, empty, or
    /// if no valid user records could be parsed.
    /// @param filePath  Source path.
    /// @param users     [out] Parsed user database.
    /// @return true if at least one user was loaded.
    std::string content;
    if (!loadEncryptedFile(filePath, content)) return false;

    users.clear();
    std::istringstream file(content);
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || startsWith(line, "#")) continue;

        std::vector<std::string> parts = split(line, '|');
        if (parts.size() < 6) continue;

        try {
            UserData user;
            user.username = parts[0];
            user.password = parts[1];
            user.role = static_cast<UserRole>(std::stoi(parts[2]));
            user.currentLevel = parts[3];
            user.posX = parseFloat(parts[4]);
            user.posY = parseFloat(parts[5]);
            if (parts.size() >= 9) {
                user.stats.totalDeaths = std::stoi(parts[6]);
                user.stats.totalPlaytime = parseFloat(parts[7]);
                user.stats.levelsCompleted = std::stoi(parts[8]);
            }
            users.push_back(user);
        } catch (...) { continue; }
    }
    return !users.empty();
}

bool FileSerializer::saveLevel(const Gameplay::EntityManager &entityManager, const std::string &filePath) {
    /// Serialises the entire EntityManager state into a plaintext .level file.
    /// Writes levelWidth, wall-clamp config, and every living entity in the
    /// ENTITY:Type|x|y|w|h|textureID|zIndex|extra format.
    /// Extra fields: speed (Player), parallax (BackgroundLayer),
    /// collider type (solid/trigger/hazard / exit_east/exit_west/exit_both with path+duration).
    /// @param entityManager  The manager whose entities to save.
    /// @param filePath       Destination path.
    /// @return true on success, false if the file could not be opened.
    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    file << "# Level Data - 2D Platformer Game\n";
    file << "# Custom Level Format\n\n";
    file << "levelWidth=" << floatToString(entityManager.levelConfig.levelWidth) << "\n";
    file << "isLeftWallClamped=" << (entityManager.levelConfig.isLeftWallClamped ? "true" : "false") << "\n";
    file << "isRightWallClamped=" << (entityManager.levelConfig.isRightWallClamped ? "true" : "false") << "\n\n";

    for (const auto &entity : entityManager.getEntities()) {
        if (entity->isDestroyed) continue;

        std::string typeStr = "StaticObject";
        if (std::dynamic_pointer_cast<Gameplay::Player>(entity)) typeStr = "Player";
        else if (std::dynamic_pointer_cast<Gameplay::BackgroundLayer>(entity)) typeStr = "BackgroundLayer";

        file << "ENTITY:" << typeStr << "|"
             << floatToString(entity->transform.x) << "|"
             << floatToString(entity->transform.y) << "|"
             << floatToString(entity->transform.width) << "|"
             << floatToString(entity->transform.height) << "|"
             << static_cast<int>(entity->sprite ? entity->sprite->textureID : Common::TextureID::TEX_PLATFORM) << "|"
             << entity->transform.zIndex << "|";

        if (auto player = std::dynamic_pointer_cast<Gameplay::Player>(entity)) {
            file << floatToString(player->playerControl ? player->playerControl->speed : Common::PLAYER_SPEED);
        } else if (auto bg = std::dynamic_pointer_cast<Gameplay::BackgroundLayer>(entity)) {
            file << floatToString(bg->parallax ? bg->parallax->factor : 1.0f);
        } else if (entity->exitEast.has_value() && entity->exitWest.has_value()) {
            file << "exit_both|"
                 << entity->exitEast->nextLevelPath << "|"
                 << floatToString(entity->exitEast->transitionDuration) << "|"
                 << entity->exitWest->nextLevelPath << "|"
                 << floatToString(entity->exitWest->transitionDuration);
        } else if (entity->exitEast.has_value()) {
            file << "exit_east|"
                 << entity->exitEast->nextLevelPath << "|"
                 << floatToString(entity->exitEast->transitionDuration);
        } else if (entity->exitWest.has_value()) {
            file << "exit_west|"
                 << entity->exitWest->nextLevelPath << "|"
                 << floatToString(entity->exitWest->transitionDuration);
        } else if (entity->collider.has_value()) {
            if (entity->collider->isHazard)
                file << "hazard";
            else
                file << (entity->collider->isSolid ? "solid" : entity->collider->isTrigger ? "trigger" : "none");
        } else {
            file << "none";
        }
        file << "\n";
    }

    file.close();
    return file.good();
}

} // namespace CLI
