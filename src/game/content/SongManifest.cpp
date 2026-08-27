#include "game/content/SongManifest.hpp"

#include <charconv>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pumpdx::content {

namespace {

[[nodiscard]] std::string Trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

[[nodiscard]] std::string Required(
    const std::unordered_map<std::string, std::string>& values,
    const std::string_view key) {
    const auto found = values.find(std::string(key));
    if (found == values.end() || found->second.empty()) {
        throw std::runtime_error("Song manifest is missing required key: " + std::string(key));
    }
    return found->second;
}

[[nodiscard]] std::string Optional(
    const std::unordered_map<std::string, std::string>& values,
    const std::string_view key) {
    const auto found = values.find(std::string(key));
    return found == values.end() ? std::string{} : found->second;
}

template <typename Integer>
[[nodiscard]] Integer ParseInteger(const std::string& value, const std::string_view key) {
    Integer result{};
    const auto [position, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc{} || position != value.data() + value.size()) {
        throw std::runtime_error("Song manifest has invalid integer key: " + std::string(key));
    }
    return result;
}

[[nodiscard]] double ParseDouble(const std::string& value, const std::string_view key) {
    try {
        std::size_t parsedLength = 0;
        const auto result = std::stod(value, &parsedLength);
        if (parsedLength != value.size()) {
            throw std::runtime_error("invalid trailing data");
        }
        return result;
    } catch (const std::exception&) {
        throw std::runtime_error("Song manifest has invalid decimal key: " + std::string(key));
    }
}

} // namespace

SongManifest SongManifest::Load(const std::filesystem::path& manifestPath) {
    std::ifstream input(manifestPath);
    if (!input) {
        throw std::runtime_error("Unable to open song manifest: " + manifestPath.string());
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(input, line)) {
        line = Trim(std::move(line));
        if (line.empty() || line.starts_with('#')) {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::runtime_error("Song manifest lines must use key=value syntax.");
        }
        const auto key = Trim(line.substr(0, separator));
        const auto value = Trim(line.substr(separator + 1));
        if (key.empty() || !values.emplace(key, value).second) {
            throw std::runtime_error("Song manifest has an empty or duplicate key.");
        }
    }

    if (ParseInteger<int>(Required(values, "schemaVersion"), "schemaVersion") != 1) {
        throw std::runtime_error("Unsupported song manifest schema version.");
    }

    const auto level = ParseInteger<int>(Required(values, "difficultyLevel"), "difficultyLevel");
    if (level < 1 || level > 99) {
        throw std::runtime_error("Song manifest difficultyLevel must be between 1 and 99.");
    }

    const auto offsetMilliseconds = ParseDouble(Required(values, "audioOffsetMilliseconds"), "audioOffsetMilliseconds");
    const auto legacyStartPosition = ParseInteger<int>(Required(values, "legacyStartPosition"), "legacyStartPosition");
    if (legacyStartPosition < 83) {
        throw std::runtime_error("Song manifest legacyStartPosition must be at least 83.");
    }
    const auto title = Required(values, "title");
    const auto artist = Required(values, "artist");
    const auto difficultyName = Required(values, "difficultyName");
    return {
        .metadata = {
            .id = Required(values, "id"),
            .title = std::wstring(title.begin(), title.end()),
            .artist = std::wstring(artist.begin(), artist.end()),
            .difficultyName = std::wstring(difficultyName.begin(), difficultyName.end()),
            .difficultyLevel = static_cast<std::uint8_t>(level),
        },
        .audioFilePath = (manifestPath.parent_path() / Required(values, "audioPath")).lexically_normal(),
        .chartFilePath = (manifestPath.parent_path() / Required(values, "chartPath")).lexically_normal(),
        .holdOverlayFilePath = [&manifestPath, &values] {
            const auto overlayPath = Optional(values, "holdOverlayPath");
            return overlayPath.empty()
                ? std::filesystem::path{}
                : (manifestPath.parent_path() / overlayPath).lexically_normal();
        }(),
        .staticBgaFilePath = [&manifestPath, &values] {
            const auto bgaPath = Optional(values, "staticBgaPath");
            return bgaPath.empty()
                ? std::filesystem::path{}
                : (manifestPath.parent_path() / bgaPath).lexically_normal();
        }(),
        .audioOffsetSeconds = offsetMilliseconds / 1000.0,
        .legacyStartPosition = legacyStartPosition,
    };
}

} // namespace pumpdx::content
