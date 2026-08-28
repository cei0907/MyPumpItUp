#pragma once

#include "game/chart/Chart.hpp"
#include "game/content/SongMetadata.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace pumpdx::content {

struct SongDefinition final {
    SongMetadata metadata;
    std::shared_ptr<const chart::Chart> chart;
    std::filesystem::path audioFilePath;
    std::filesystem::path staticBgaFilePath;
    std::filesystem::path videoBgaFilePath;
    double audioOffsetSeconds = 0.0;
};

class SongCatalog final {
public:
    explicit SongCatalog(std::vector<SongDefinition> songs);

    [[nodiscard]] static SongCatalog CreateDemoCatalog(const std::filesystem::path& manifestPath);
    [[nodiscard]] static SongCatalog CreateFromManifestDirectory(const std::filesystem::path& directoryPath);
    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] std::size_t DifficultyCount(std::size_t songIndex) const;
    [[nodiscard]] const SongDefinition& At(std::size_t songIndex, std::size_t difficultyIndex = 0) const;

private:
    [[nodiscard]] static std::shared_ptr<const chart::Chart> CreateFallbackChart();
    [[nodiscard]] static SongDefinition LoadDefinition(const std::filesystem::path& manifestPath);

    std::vector<std::vector<SongDefinition>> songs_;
};

} // namespace pumpdx::content
