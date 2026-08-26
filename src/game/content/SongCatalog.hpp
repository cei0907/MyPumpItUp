#pragma once

#include "game/content/SongMetadata.hpp"

#include <cstddef>
#include <vector>

namespace pumpdx::content {

class SongCatalog final {
public:
    explicit SongCatalog(std::vector<SongMetadata> songs);

    [[nodiscard]] static SongCatalog CreateDemoCatalog();
    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] const SongMetadata& At(std::size_t index) const;

private:
    std::vector<SongMetadata> songs_;
};

} // namespace pumpdx::content
