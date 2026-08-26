#include "game/session/PlaySession.hpp"

#include <stdexcept>
#include <utility>

namespace pumpdx::session {

PlaySession::PlaySession(content::SongMetadata selectedSong, std::shared_ptr<const chart::Chart> selectedChart)
    : selectedSong_(std::move(selectedSong))
    , selectedChart_(std::move(selectedChart)) {
    if (selectedChart_ == nullptr) {
        throw std::invalid_argument("A play session requires a chart.");
    }
}

const content::SongMetadata& PlaySession::SelectedSong() const noexcept {
    return selectedSong_;
}

const chart::Chart& PlaySession::SelectedChart() const noexcept {
    return *selectedChart_;
}

ResultData PlaySession::BuildResult(GameplaySummary summary) const {
    return {
        .song = selectedSong_,
        .summary = summary,
    };
}

} // namespace pumpdx::session
