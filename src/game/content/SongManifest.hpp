#pragma once

#include "game/content/SongMetadata.hpp"

#include <filesystem>

namespace pumpdx::content {

struct SongManifest final {
    SongMetadata metadata;
    std::filesystem::path audioFilePath;
    double audioOffsetSeconds = 0.0;

    [[nodiscard]] static SongManifest Load(const std::filesystem::path& manifestPath);
};

} // namespace pumpdx::content
