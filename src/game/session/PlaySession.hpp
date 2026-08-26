#pragma once

#include "game/content/SongMetadata.hpp"
#include "game/session/ResultData.hpp"

namespace pumpdx::session {

class PlaySession final {
public:
    explicit PlaySession(content::SongMetadata selectedSong);

    [[nodiscard]] const content::SongMetadata& SelectedSong() const noexcept;
    [[nodiscard]] ResultData BuildResult(GameplaySummary summary) const;

private:
    content::SongMetadata selectedSong_;
};

} // namespace pumpdx::session
