#include "game/chart/LegacyStpConverter.hpp"

#include "game/chart/LegacyStpImporter.hpp"
#include "game/chart/NativeChartLoader.hpp"

#include <utility>
#include <vector>

namespace pumpdx::chart {

Chart LegacyStpConverter::Convert(
    const std::filesystem::path& legacyStpPath,
    std::string nativeChartId,
    const int legacyStartPosition,
    const std::optional<std::filesystem::path>& holdOverlayPath) {
    auto baseChart = LegacyStpImporter::LoadTapChart(
        legacyStpPath, nativeChartId + "-legacy-source", legacyStartPosition);
    auto notes = baseChart.Notes();
    if (holdOverlayPath.has_value()) {
        const auto overlayChart = NativeChartLoader::Load(*holdOverlayPath);
        notes.insert(notes.end(), overlayChart.Notes().begin(), overlayChart.Notes().end());
    }
    return Chart(std::move(nativeChartId), baseChart.Timing(), std::move(notes), baseChart.DelaySeconds());
}

} // namespace pumpdx::chart
