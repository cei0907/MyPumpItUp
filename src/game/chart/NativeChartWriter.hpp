#pragma once

#include "game/chart/Chart.hpp"

#include <filesystem>
#include <string>

namespace pumpdx::chart {

class NativeChartWriter final {
public:
    [[nodiscard]] static std::string Serialize(const Chart& chart);
    static void Save(const Chart& chart, const std::filesystem::path& destinationPath);
};

} // namespace pumpdx::chart
