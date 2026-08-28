#include "game/content/SongCatalog.hpp"

#include "game/chart/NoteEvent.hpp"
#include "game/chart/LegacyStpImporter.hpp"
#include "game/chart/NativeChartLoader.hpp"
#include "game/content/SongManifest.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace pumpdx::content {

SongCatalog::SongCatalog(std::vector<SongDefinition> songs)
{
    if (songs.empty()) {
        throw std::invalid_argument("A song catalog requires at least one song.");
    }

    std::unordered_map<std::string, std::size_t> groupIndices;
    for (auto& song : songs) {
        if (song.metadata.id.empty() || song.chart == nullptr) {
            throw std::invalid_argument("Every song requires metadata and a chart.");
        }

        const auto [found, inserted] = groupIndices.emplace(song.metadata.id, songs_.size());
        if (inserted) {
            songs_.push_back({});
        }
        auto& group = songs_.at(found->second);
        const auto duplicateDifficulty = std::ranges::any_of(group, [&song](const SongDefinition& existing) {
            return existing.metadata.difficultyLevel == song.metadata.difficultyLevel;
        });
        if (duplicateDifficulty) {
            throw std::invalid_argument("A song cannot contain duplicate difficulty levels.");
        }
        group.push_back(std::move(song));
    }
}

SongCatalog SongCatalog::CreateDemoCatalog(const std::filesystem::path& manifestPath) {
    return SongCatalog({LoadDefinition(manifestPath)});
}

SongCatalog SongCatalog::CreateFromManifestDirectory(const std::filesystem::path& directoryPath) {
    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error("Song manifest directory was not found: " + directoryPath.string());
    }

    std::vector<std::filesystem::path> manifestPaths;
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".manifest") {
            continue;
        }
        const auto fileName = entry.path().filename().string();
        if (fileName.ends_with(".song.manifest")) {
            manifestPaths.push_back(entry.path());
        }
    }
    std::ranges::sort(manifestPaths);
    if (manifestPaths.empty()) {
        throw std::runtime_error("Song manifest directory contains no .song.manifest files: " + directoryPath.string());
    }

    std::vector<SongDefinition> definitions;
    definitions.reserve(manifestPaths.size());
    for (const auto& path : manifestPaths) {
        definitions.push_back(LoadDefinition(path));
    }
    return SongCatalog(std::move(definitions));
}

SongDefinition SongCatalog::LoadDefinition(const std::filesystem::path& manifestPath) {
    const auto manifest = SongManifest::Load(manifestPath);
    const auto hasChart = std::filesystem::is_regular_file(manifest.chartFilePath);
    auto extension = manifest.chartFilePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return {
        .metadata = manifest.metadata,
        .chart = [&manifest, hasChart, &extension] {
            if (!hasChart) {
                return CreateFallbackChart();
            }
            std::shared_ptr<const chart::Chart> baseChart;
            if (extension == ".stp") {
                baseChart = std::make_shared<const chart::Chart>(chart::LegacyStpImporter::LoadTapChart(
                    manifest.chartFilePath,
                    manifest.metadata.id + "-legacy-stp",
                    manifest.legacyStartPosition));
            } else if (extension == ".pdxchart") {
                baseChart = std::make_shared<const chart::Chart>(chart::NativeChartLoader::Load(manifest.chartFilePath));
            } else {
                throw std::runtime_error("Unsupported chart extension: " + manifest.chartFilePath.extension().string());
            }

            if (manifest.holdOverlayFilePath.empty()) {
                return baseChart;
            }
            if (!std::filesystem::is_regular_file(manifest.holdOverlayFilePath)) {
                throw std::runtime_error("Hold overlay chart was not found: " + manifest.holdOverlayFilePath.string());
            }

            const auto overlayChart = chart::NativeChartLoader::Load(manifest.holdOverlayFilePath);
            auto mergedNotes = baseChart->Notes();
            const auto& overlayNotes = overlayChart.Notes();
            mergedNotes.insert(mergedNotes.end(), overlayNotes.begin(), overlayNotes.end());
            return std::make_shared<const chart::Chart>(
                baseChart->Id() + "-hold-overlay",
                baseChart->Timing(),
                std::move(mergedNotes));
        }(),
        .audioFilePath = manifest.audioFilePath,
        .staticBgaFilePath = manifest.staticBgaFilePath,
        .videoBgaFilePath = manifest.videoBgaFilePath,
        .audioOffsetSeconds = manifest.audioOffsetSeconds,
    };
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

std::size_t SongCatalog::DifficultyCount(const std::size_t songIndex) const {
    if (songIndex >= songs_.size()) {
        throw std::out_of_range("Song index is outside the catalog.");
    }
    return songs_.at(songIndex).size();
}

const SongDefinition& SongCatalog::At(const std::size_t songIndex, const std::size_t difficultyIndex) const {
    if (songIndex >= songs_.size() || difficultyIndex >= songs_.at(songIndex).size()) {
        throw std::out_of_range("Song or difficulty index is outside the catalog.");
    }
    return songs_.at(songIndex).at(difficultyIndex);
}

} // namespace pumpdx::content
