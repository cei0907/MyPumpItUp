#pragma once

#include "game/chart/Chart.hpp"
#include "game/content/SongMetadata.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace pumpdx::content {

struct SongDefinition final {
    SongMetadata metadata;
    std::shared_ptr<const chart::Chart> chart;
};

class SongCatalog final {
public:
    explicit SongCatalog(std::vector<SongDefinition> songs);

    [[nodiscard]] static SongCatalog CreateDemoCatalog();
    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] const SongDefinition& At(std::size_t index) const;

private:
    std::vector<SongDefinition> songs_;
};

} // namespace pumpdx::content
