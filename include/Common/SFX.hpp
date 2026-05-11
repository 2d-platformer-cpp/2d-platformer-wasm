/// @file SFX.hpp
/// Enumeration of all sound effect identifiers used by the game.
/// Each value maps to a WAV file loaded at startup by AudioManager.

#pragma once

namespace Common {

/// Identifiers for every sound effect used by the game.
/// Each value maps to a WAV file loaded by AudioManager.
enum class SFX : int {
    RUN_LOOP,          ///< Footstep loop played while player runs on ground
    LANDING,           ///< One-shot on transition from airborne to grounded
    DEATH_SCREAM,      ///< One-shot on player death (hazard or fall)
    RESPAWN,           ///< One-shot when the level reloads after death or transition
    IDLE_BREATHING,    ///< Periodic breathing sound while player stands still
    HEAVY_BREATHING,   ///< Played after sustained running then coming to rest
    WIND_GRASS,        ///< Ambient loop started when entering RUNNING state from menu
    BIRDS_CHIRP,       ///< Ambient bird chirp triggered at random intervals
    LAVA,              ///< Looping fire/crackle sound when player is near lava
    COUNT              ///< Sentinel: total number of SFX IDs
};

} // namespace Common
