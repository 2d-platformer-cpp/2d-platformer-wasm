/// @file Utilities.hpp
/// Shared string utility functions: trimming, splitting, parsing, and formatting.
/// Used by LevelLoader and FileSerializer to avoid code duplication.

#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

namespace Common { namespace Util {

/// Strips leading and trailing whitespace (space, tab, newline, carriage-return).
/// @param str  Input string to trim.
/// @return A new string with whitespace removed from both ends.
inline std::string trim(const std::string &str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

/// Splits a string on every occurrence of a delimiter character.
/// Empty tokens between consecutive delimiters are kept.
/// @param str        Input string.
/// @param delimiter  Character to split on.
/// @return A vector of token strings.
inline std::vector<std::string> split(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter))
        tokens.push_back(token);
    return tokens;
}

/// Checks if a string starts with a given prefix.
/// @param str     The string to examine.
/// @param prefix  The prefix to look for.
/// @return true if str starts with prefix, false otherwise.
inline bool startsWith(const std::string &str, const std::string &prefix) {
    return str.starts_with(prefix);
}

/// Parses a boolean from common truthy string representations.
/// Accepts "true", "1", "yes" (case-insensitive). Everything else is false.
/// @param str  The string to parse.
/// @return The parsed boolean value.
inline bool parseBool(const std::string &str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return (lower == "true" || lower == "1" || lower == "yes");
}

/// Parses a float from a string using std::stof.
/// Returns 0.0f on any parse failure.
/// @param str  The string to parse.
/// @return The parsed float value.
inline float parseFloat(const std::string &str) {
    try { return std::stof(str); }
    catch (const std::invalid_argument&) { return 0.0f; }
    catch (const std::out_of_range&) { return 0.0f; }
}

inline int parseInt(const std::string &str) {
    try { return std::stoi(str); }
    catch (const std::invalid_argument&) { return 0; }
    catch (const std::out_of_range&) { return 0; }
}

/// Formats a float as a fixed-precision 6-decimal-string.
/// Used for serializing positions, times, etc. into level files.
/// @param value  The float to format.
/// @return A string like "123.456789".
inline std::string floatToString(float value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << value;
    return oss.str();
}

}} // namespace Common::Util
