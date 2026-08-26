#include "game/content/SongCatalog.hpp"

#include <stdexcept>
#include <utility>

namespace pumpdx::content {

SongCatalog::SongCatalog(std::vector<SongMetadata> songs)
    : songs_(std::move(songs)) {
    if (songs_.empty()) {
        throw std::invalid_argument("A song catalog requires at least one song.");
    }
}

SongCatalog SongCatalog::CreateDemoCatalog() {
    return SongCatalog({
        {
            .id = "state-03-demo",
            .title = L"State 03 Demo",
            .artist = L"PumpDX Rebuild",
            .difficultyName = L"Foundation",
            .difficultyLevel = 1,
        },
    });
}

std::size_t SongCatalog::Count() const noexcept {
    return songs_.size();
}

const SongMetadata& SongCatalog::At(const std::size_t index) const {
    if (index >= songs_.size()) {
        throw std::out_of_range("Song index is outside the catalog.");
    }

    return songs_[index];
}

} // namespace pumpdx::content
