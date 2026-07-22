#pragma once

// ConfigReader.h - Shared JSON/XML configuration reading utilities
//
// Header-only helpers standardizing file I/O patterns across all projects.
// Requires nlohmann_json (available as workspace-level xmake dependency).
//
// Usage:
//   #include <ConfigReader.h>
//
//   // Simple JSON file read
//   auto j = ConfigReader::TryLoadJsonFile("Data/settings.json");
//   if (j) { /* use *j */ }
//
//   // Iterate all .json files in a directory
//   ConfigReader::ForEachJsonInDirectory("Data/F4SE/Plugins/MyMod",
//       [](auto& j, auto& path) { /* process *j */ });

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <optional>

namespace ConfigReader {

// Try to open and parse a JSON file.
// Returns std::nullopt on any error (file not found, invalid JSON, etc.)
inline std::optional<nlohmann::json> TryLoadJsonFile(const std::filesystem::path& a_path)
{
    try {
        std::ifstream file(a_path);
        if (!file.is_open()) {
            return std::nullopt;
        }
        return nlohmann::json::parse(file, nullptr, true, true);
    }
    catch (...) {
        return std::nullopt;
    }
}

// Iterate over all .json files in a directory and call a callback for each.
//
// @param a_dir       Directory path to scan.
// @param a_callback  Invoked as (const nlohmann::json&, const std::filesystem::path&)
//                    for each successfully parsed JSON file.
// @param a_recursive If true, use recursive_directory_iterator.
// @return Number of successfully parsed files.
template <typename F>
size_t ForEachJsonInDirectory(
    const std::filesystem::path& a_dir,
    F&& a_callback,
    bool a_recursive = false)
{
    if (!std::filesystem::exists(a_dir)) {
        return 0;
    }

    size_t count = 0;
    try {
        auto processEntry = [&](const auto& entry) {
            if (entry.path().extension() != ".json") {
                return;
            }
            auto j = TryLoadJsonFile(entry.path());
            if (j) {
                a_callback(*j, entry.path());
                count++;
            }
            };

        if (a_recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(a_dir)) {
                processEntry(entry);
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(a_dir)) {
                processEntry(entry);
            }
        }
    }
    catch (const std::filesystem::filesystem_error&) {
        // Directory may have been deleted/renamed during iteration
    }

    return count;
}

} // namespace ConfigReader
