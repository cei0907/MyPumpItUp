#pragma once

#include "game/chart/Chart.hpp"

#include <filesystem>

namespace pumpdx::chart {

// Loads the readable, versioned .pdxchart source format authored by the future chart editor.
class NativeChartLoader final {
public:
    [[nodiscard]] static Chart Load(const std::filesystem::path& sourcePath);
};

} // namespace pumpdx::chart
