#pragma once

#include "game/chart/Chart.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace pumpdx::chart {

class LegacyStpConverter final {
public:
    [[nodiscard]] static Chart Convert(
        const std::filesystem::path& legacyStpPath,
        std::string nativeChartId,
        int legacyStartPosition,
        const std::optional<std::filesystem::path>& holdOverlayPath = std::nullopt);
};

} // namespace pumpdx::chart
