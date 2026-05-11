/// @file StatsManager.hpp
/// Tracks per-level and cumulative player statistics: deaths, playtime,
/// best times, and completion state. Integrates with AuthService for persistence.

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Gameplay {

/// Tracks per-level and cumulative player statistics.
/// Maintains death counts, playtime, best times, and completion state.
/// Integrates with AuthService for persistence between sessions.
class StatsManager {
public:
    /// Called when a level is started (or restarted).
    /// Resets per-level counters and records the level path.
    /// @param levelPath  Path to the level being started.
    void onLevelStart(const std::string &levelPath);

    /// Called when the player dies. Increments both current-level and total death counts.
    void onDeath();

    /// Called when the player reaches a level exit.
    /// Records completion time/deaths and updates best time if improved.
    /// @param levelPath  The completed level.
    void onLevelComplete(const std::string &levelPath);

    /// Accumulates playtime. Called each fixed timestep.
    /// @param deltaTime  Time step in seconds.
    void update(float deltaTime);

    // --- Per-level accessors ---
    int getCurrentDeaths() const { return m_currentLevelDeaths; }        ///< Deaths in the current level.
    float getCurrentLevelTime() const { return m_currentLevelTime; }      ///< Time spent in the current level.
    const std::string& getCurrentLevel() const { return m_currentLevel; } ///< Path of the current level.

    // --- Cumulative accessors ---
    int getTotalDeaths() const { return m_totalDeaths; }                  ///< Total deaths across all sessions.
    float getTotalPlaytime() const { return m_totalPlaytime; }            ///< Cumulative playtime.
    int getLevelsCompleted() const { return m_levelsCompleted; }          ///< Number of distinct levels completed.
    void setTotalDeaths(int deaths) { m_totalDeaths = deaths; }           ///< Restore from persistent storage.
    void setTotalPlaytime(float time) { m_totalPlaytime = time; }         ///< Restore from persistent storage.
    void setLevelsCompleted(int completed) { m_levelsCompleted = completed; } ///< Restore from persistent storage.

    // --- Completion tracking ---
    bool hasCompleted(const std::string &levelPath) const;                ///< Whether a level was ever completed.
    void setCompletedLevels(const std::vector<std::string> &levels);      ///< Restore completed list.
    std::vector<std::string> getCompletedLevels() const;                  ///< Get completed list for persistence.

    // --- Best times ---
    float getBestTime(const std::string &levelPath) const;                ///< Best completion time, or INF.
    void setBestTimes(const std::unordered_map<std::string, float> &times);     ///< Restore best times.
    std::unordered_map<std::string, float> getBestTimes() const;                ///< Get best times for persistence.

    // --- Last-completed-level accessors ---
    int getLastCompletedDeaths() const { return m_lastCompletedDeaths; }   ///< Deaths on the most recently completed run.
    float getLastCompletedTime() const { return m_lastCompletedTime; }     ///< Time of the most recently completed run.
    const std::string& getLastCompletedLevel() const { return m_lastCompletedLevel; }  ///< Path of the last completed level.
    bool getLastCompletedNewBest() const { return m_lastCompletedNewBest; }  ///< Whether the last run set a new best time.

    /// Formats a time value (seconds) as "M:SS.mm" for HUD display.
    /// @param seconds  Time in seconds.
    /// @return Formatted string.
    static std::string formatTime(float seconds);

private:
    std::string m_currentLevel;                ///< Current level path.
    int m_currentLevelDeaths = 0;              ///< Deaths in current level.
    int m_totalDeaths = 0;                     ///< Cumulative total deaths.
    float m_currentLevelTime = 0.0f;           ///< Time in current level.
    float m_totalPlaytime = 0.0f;              ///< Cumulative playtime.
    int m_levelsCompleted = 0;                 ///< Number of completed levels.

    std::unordered_map<std::string, float> m_bestTimes;       ///< Best times per level (levelPath → seconds).
    std::unordered_set<std::string> m_completedLevels;        ///< Set of completed level paths.

    int m_lastCompletedDeaths = 0;             ///< Deaths on the last completed level.
    float m_lastCompletedTime = 0.0f;          ///< Time on the last completed level.
    std::string m_lastCompletedLevel;           ///< Path of the last completed level.
    bool m_lastCompletedNewBest = false;        ///< Whether the last completion beat the old best.
};

} // namespace Gameplay
