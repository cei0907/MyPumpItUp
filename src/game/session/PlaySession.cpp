#include "game/session/PlaySession.hpp"

#include <utility>

namespace pumpdx::session {

PlaySession::PlaySession(content::SongMetadata selectedSong)
    : selectedSong_(std::move(selectedSong)) {
}

const content::SongMetadata& PlaySession::SelectedSong() const noexcept {
    return selectedSong_;
}

ResultData PlaySession::BuildResult(GameplaySummary summary) const {
    return {
        .song = selectedSong_,
        .summary = summary,
    };
}

} // namespace pumpdx::session
