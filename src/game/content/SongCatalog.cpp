#include "game/content/SongCatalog.hpp"

#include "game/chart/NoteEvent.hpp"
#include "game/chart/LegacyStpImporter.hpp"
#include "game/chart/NativeChartLoader.hpp"
#include "game/content/SongManifest.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <utility>

namespace pumpdx::content {

SongCatalog::SongCatalog(std::vector<SongDefinition> songs)
    : songs_(std::move(songs)) {
    if (songs_.empty()) {
        throw std::invalid_argument("A song catalog requires at least one song.");
    }
    for (const auto& song : songs_) {
        if (song.metadata.id.empty() || song.chart == nullptr) {
            throw std::invalid_argument("Every song requires metadata and a chart.");
        }
    }
}

SongCatalog SongCatalog::CreateDemoCatalog(const std::filesystem::path& manifestPath) {
    const auto manifest = SongManifest::Load(manifestPath);
    const auto hasChart = std::filesystem::is_regular_file(manifest.chartFilePath);
    auto extension = manifest.chartFilePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return SongCatalog({
        {
            .metadata = manifest.metadata,
            .chart = [&manifest, hasChart, &extension] {
                if (!hasChart) {
                    return CreateFallbackChart();
                }
                if (extension == ".stp") {
                    return std::make_shared<const chart::Chart>(chart::LegacyStpImporter::LoadTapChart(
                        manifest.chartFilePath,
                        manifest.metadata.id + "-legacy-stp",
                        manifest.legacyStartPosition));
                }
                if (extension == ".pdxchart") {
                    return std::make_shared<const chart::Chart>(chart::NativeChartLoader::Load(manifest.chartFilePath));
                }
                throw std::runtime_error("Unsupported chart extension: " + manifest.chartFilePath.extension().string());
            }(),
            .audioFilePath = manifest.audioFilePath,
            .staticBgaFilePath = manifest.staticBgaFilePath,
            .audioOffsetSeconds = manifest.audioOffsetSeconds,
        },
    });
}

std::shared_ptr<const chart::Chart> SongCatalog::CreateFallbackChart() {
    return std::make_shared<const chart::Chart>(
        "fallback-foundation-chart",
        chart::TimingMap({{{0}, 120.0}}),
        std::vector<chart::NoteEvent>{
            chart::TapNote{.beat = {0}, .lane = chart::PanelLane::DownLeft},
            chart::TapNote{.beat = {1}, .lane = chart::PanelLane::UpLeft},
        });
}

std::size_t SongCatalog::Count() const noexcept {
    return songs_.size();
}

const SongDefinition& SongCatalog::At(const std::size_t index) const {
    if (index >= songs_.size()) {
        throw std::out_of_range("Song index is outside the catalog.");
    }

    return songs_[index];
}

} // namespace pumpdx::content
