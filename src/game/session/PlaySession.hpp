#pragma once

#include "game/chart/Chart.hpp"
#include "game/content/SongMetadata.hpp"
#include "game/session/ResultData.hpp"

#include <memory>

namespace pumpdx::session {

class PlaySession final {
public:
    PlaySession(content::SongMetadata selectedSong, std::shared_ptr<const chart::Chart> selectedChart);

    [[nodiscard]] const content::SongMetadata& SelectedSong() const noexcept;
    [[nodiscard]] const chart::Chart& SelectedChart() const noexcept;
    [[nodiscard]] ResultData BuildResult(GameplaySummary summary) const;

private:
    content::SongMetadata selectedSong_;
    std::shared_ptr<const chart::Chart> selectedChart_;
};

} // namespace pumpdx::session
