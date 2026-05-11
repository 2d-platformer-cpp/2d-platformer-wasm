/// @file LevelLoader.hpp
/// Static parser for .level files. Handles comment lines, config key=value pairs,
/// and entity definitions in ENTITY:Type|x|y|w|h|textureID|zIndex|extra... format.

#pragma once

#include <string>
#include "Gameplay/EntityManager.hpp"

namespace Gameplay {

/// Static parser for .level files.
/// Level file format:
///   - Comment lines start with #
///   - Config: key=value (levelWidth, isLeftWallClamped, isRightWallClamped)
///   - Entities: ENTITY:Type|x|y|w|h|textureID|zIndex|[extra...]
///     Types: Player, StaticObject, BackgroundLayer
///     Extra fields: speed (Player), parallax (BackgroundLayer),
///     collider type / exit definition (StaticObject)
class LevelLoader {
public:
    /// Reads a .level file, clears the entity manager, then populates it
    /// with parsed entities and applies the level configuration.
    /// @param entityManager  The manager to populate.
    /// @param path           Filesystem path to the .level file.
    /// @return true on success, false if the file could not be opened.
    static bool loadLevel(EntityManager &entityManager, const std::string &path);
};

} // namespace Gameplay
