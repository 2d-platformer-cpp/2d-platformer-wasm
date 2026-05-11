/// @file StatsManager.cpp
/// Implementation of the StatsManager class: per-level and cumulative death
/// count, playtime tracking, best-time recording with new-best detection,
/// completion-state management, and time formatting for HUD display.

#include "Gameplay/StatsManager.hpp"
#include <cstdio>

namespace Gameplay {

void StatsManager::onLevelStart(const std::string &levelPath) {
    /// Resets per-level counters (deaths and time) and records the current
    /// level path. Called when a level is started or restarted.
    /// @param levelPath  Path to the level being started.
    m_currentLevel = levelPath;
    m_currentLevelDeaths = 0;
    m_currentLevelTime = 0.0f;
}

void StatsManager::onDeath() {
    /// Increments both the current-level death count and the cumulative
    /// total death count.
    ++m_currentLevelDeaths;
    ++m_totalDeaths;
}

void StatsManager::onLevelComplete(const std::string &levelPath) {
    /// Records the completion: stores last-completed stats, marks the level
    /// as completed (incrementing levelsCompleted if it is a first-time
    /// completion), and updates the best time if the current run was faster.
    /// @param levelPath  The completed level.
    m_lastCompletedLevel = levelPath;
    m_lastCompletedDeaths = m_currentLevelDeaths;
    m_lastCompletedTime = m_currentLevelTime;

    if (m_completedLevels.find(levelPath) == m_completedLevels.end()) {
        m_completedLevels.insert(levelPath);
        ++m_levelsCompleted;
    }

    auto it = m_bestTimes.find(levelPath);
    if (it == m_bestTimes.end() || m_currentLevelTime < it->second) {
        m_bestTimes[levelPath] = m_currentLevelTime;
        m_lastCompletedNewBest = true;
    } else {
        m_lastCompletedNewBest = false;
    }
}

bool StatsManager::hasCompleted(const std::string &levelPath) const {
    /// Checks whether a given level has ever been completed.
    /// @param levelPath  The level path to check.
    /// @return true if the level is in the completed set.
    return m_completedLevels.find(levelPath) != m_completedLevels.end();
}

void StatsManager::setCompletedLevels(const std::vector<std::string> &levels) {
    /// Restores the completed-levels set from persistent storage.
    /// Recalculates levelsCompleted from the set size.
    /// @param levels  List of completed level paths.
    m_completedLevels.clear();
    m_completedLevels.insert(levels.begin(), levels.end());
    m_levelsCompleted = static_cast<int>(m_completedLevels.size());
}

std::vector<std::string> StatsManager::getCompletedLevels() const {
    /// Returns the list of completed level paths for persistence.
    /// @return Vector of completed level paths.
    return std::vector<std::string>(m_completedLevels.begin(), m_completedLevels.end());
}

void StatsManager::update(float deltaTime) {
    /// Accumulates playtime for both the current level and the total session.
    /// Called each fixed timestep.
    /// @param deltaTime  Time step in seconds.
    m_currentLevelTime += deltaTime;
    m_totalPlaytime += deltaTime;
}

float StatsManager::getBestTime(const std::string &levelPath) const {
    /// Returns the best completion time for a given level, or 0.0f if
    /// the level has never been completed.
    /// @param levelPath  The level path to look up.
    /// @return Best time in seconds, or 0.0f.
    auto it = m_bestTimes.find(levelPath);
    if (it != m_bestTimes.end())
        return it->second;
    return 0.0f;
}

void StatsManager::setBestTimes(const std::unordered_map<std::string, float> &times) {
    /// Restores the best-times map from persistent storage.
    /// @param times  Map of levelPath → best time in seconds.
    m_bestTimes = times;
}

std::unordered_map<std::string, float> StatsManager::getBestTimes() const {
    /// Returns the best-times map for persistence.
    /// @return Map of levelPath → best time in seconds.
    return m_bestTimes;
}

std::string StatsManager::formatTime(float seconds) {
    /// Formats a time value (in seconds) as "MM:SS" for HUD display.
    /// Clamps negative values to zero.
    /// @param seconds  Time in seconds.
    /// @return Formatted string like "01:23".
    int totalSec = static_cast<int>(seconds);
    int mins = totalSec / 60;
    int secs = totalSec % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
    return std::string(buf);
}

} // namespace Gameplay
