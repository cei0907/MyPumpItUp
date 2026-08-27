#pragma once

#include "game/content/SongMetadata.hpp"

#include <filesystem>

namespace pumpdx::content {

struct SongManifest final {
    SongMetadata metadata;
    std::filesystem::path audioFilePath;
    std::filesystem::path chartFilePath;
    double audioOffsetSeconds = 0.0;
    int legacyStartPosition = 83;

    [[nodiscard]] static SongManifest Load(const std::filesystem::path& manifestPath);
};

} // namespace pumpdx::content
