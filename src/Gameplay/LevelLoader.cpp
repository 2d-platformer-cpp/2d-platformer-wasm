/// @file LevelLoader.cpp
/// Implementation of the LevelLoader class: parses .level files, handles
/// comment lines (starting with #), config key=value pairs (levelWidth,
/// isLeftWallClamped, isRightWallClamped), and entity definitions in the
/// format ENTITY:Type|x|y|w|h|textureID|zIndex|extra...

#include "Gameplay/LevelLoader.hpp"
#include "Gameplay/Entity.hpp"
#include "Common/Constants.hpp"
#include "Common/Utilities.hpp"

#include <fstream>
#include <sstream>

namespace Gameplay {

using namespace Common::Util;

namespace {
    /// Parses a TextureID from a string integer, clamping to valid range.
    /// Falls back to TEX_PLATFORM (-1) if the value is out of range.
    /// @param str  The string to parse.
    /// @return The corresponding TextureID.
    Common::TextureID parseTextureID(const std::string &str) {
        int id = parseInt(str);
        int max = static_cast<int>(Common::TextureID::TEX_COUNT) - 1;
        if (id < 0 || id > max) id = static_cast<int>(Common::TextureID::TEX_PLATFORM);
        return static_cast<Common::TextureID>(id);
    }
}

bool LevelLoader::loadLevel(EntityManager &entityManager, const std::string &path) {
    /// Reads a .level file line by line: trims whitespace, skips empty lines
    /// and comments, processes config key=value pairs, and parses entity lines.
    /// Creates Player, StaticObject, or BackgroundLayer entities as appropriate
    /// and populates their components from pipe-delimited fields.
    /// StaticObject supports extra types: solid, trigger, hazard, exit_east,
    /// exit_west, exit_both (with path and duration fields).
    /// @param entityManager  The manager to populate.
    /// @param path           Filesystem path to the .level file.
    /// @return true on success, false if the file could not be opened.
    std::ifstream file(path);
    if (!file.is_open()) return false;

    entityManager.clear();

    entityManager.levelConfig.isLeftWallClamped = true;
    entityManager.levelConfig.isRightWallClamped = true;
    entityManager.levelConfig.levelWidth = Common::DEFAULT_LEVEL_WIDTH;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || startsWith(line, "#")) continue;

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos && !startsWith(line, "ENTITY:")) {
            std::string key = trim(line.substr(0, eqPos));
            std::string value = trim(line.substr(eqPos + 1));
            if (key == "levelWidth") entityManager.levelConfig.levelWidth = parseFloat(value);
            else if (key == "isLeftWallClamped") entityManager.levelConfig.isLeftWallClamped = parseBool(value);
            else if (key == "isRightWallClamped") entityManager.levelConfig.isRightWallClamped = parseBool(value);
            continue;
        }

        if (!startsWith(line, "ENTITY:")) continue;

        std::vector<std::string> parts = split(line, '|');
        if (parts.size() < 8) continue;

        try {
            std::string typeStr = trim(parts[0].substr(7));
            std::shared_ptr<Entity> entity;

            if (typeStr == "Player") {
                auto player = std::make_shared<Player>();
                player->transform = {parseFloat(parts[1]), parseFloat(parts[2]),
                                     parseFloat(parts[3]), parseFloat(parts[4]), parseInt(parts[6])};
                player->sprite = {parseTextureID(parts[5])};
                player->playerControl = {parseFloat(parts[7])};
                player->physics = Physics();
                if (player->playerControl)
                    player->physics->maxSpeed = player->playerControl->speed;
                player->collider = {player->transform.width, player->transform.height, 0, 0, false, true};
                entity = player;
            } else if (typeStr == "StaticObject") {
                auto obj = std::make_shared<StaticObject>();
                obj->transform = {parseFloat(parts[1]), parseFloat(parts[2]),
                                  parseFloat(parts[3]), parseFloat(parts[4]), parseInt(parts[6])};
                obj->sprite = {parseTextureID(parts[5])};

                std::string extra = trim(parts[7]);
                if (extra == "exit_both" && parts.size() >= 12) {
                    obj->collider = {obj->transform.width, obj->transform.height, 0, 0, true, false};
                    obj->exitEast = {CardinalDirection::East, parts[8], parseFloat(parts[9])};
                    obj->exitWest = {CardinalDirection::West, parts[10], parseFloat(parts[11])};
                } else if (parts.size() >= 10 && (extra == "exit" || extra == "exit_east" || extra == "exit_west")) {
                    obj->collider = {obj->transform.width, obj->transform.height, 0, 0, true, false};
                    if (extra == "exit_west")
                        obj->exitWest = {CardinalDirection::West, parts[8], parseFloat(parts[9])};
                    else
                        obj->exitEast = {CardinalDirection::East, parts[8], parseFloat(parts[9])};
                } else if (extra == "solid" || extra == "trigger" || extra == "hazard") {
                    obj->collider = {obj->transform.width, obj->transform.height, 0, 0,
                                     extra == "trigger" || extra == "hazard",
                                     extra == "solid",
                                     extra == "hazard"};
                }
                entity = obj;
            } else if (typeStr == "BackgroundLayer") {
                auto bg = std::make_shared<BackgroundLayer>();
                bg->transform = {parseFloat(parts[1]), parseFloat(parts[2]),
                                 parseFloat(parts[3]), parseFloat(parts[4]), parseInt(parts[6])};
                bg->sprite = {parseTextureID(parts[5])};
                bg->parallax = {parseFloat(parts[7])};
                entity = bg;
            }

            if (entity) entityManager.addEntity(entity);
        } catch (...) { continue; }
    }

    file.close();
    return true;
}

} // namespace Gameplay
