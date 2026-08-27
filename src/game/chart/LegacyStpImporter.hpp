#pragma once

#include "game/chart/Chart.hpp"

#include <filesystem>
#include <string>

namespace pumpdx::chart {

class LegacyStpImporter final {
public:
    [[nodiscard]] static Chart LoadTapChart(
        const std::filesystem::path& sourcePath,
        std::string chartId,
        int legacyStartPosition = 83);
};

} // namespace pumpdx::chart
