#pragma once

#include "game/chart/Chart.hpp"
#include "game/content/SongMetadata.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

namespace pumpdx::content {

struct SongDefinition final {
    SongMetadata metadata;
    std::shared_ptr<const chart::Chart> chart;
    std::filesystem::path audioFilePath;
    double audioOffsetSeconds = 0.0;
};

class SongCatalog final {
public:
    explicit SongCatalog(std::vector<SongDefinition> songs);

    [[nodiscard]] static SongCatalog CreateDemoCatalog(const std::filesystem::path& manifestPath);
    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] const SongDefinition& At(std::size_t index) const;

private:
    [[nodiscard]] static std::shared_ptr<const chart::Chart> CreateFallbackChart();

    std::vector<SongDefinition> songs_;
};

} // namespace pumpdx::content
