#pragma once

#include "game/content/SongMetadata.hpp"

#include <cstdint>

namespace pumpdx::session {

struct GameplaySummary final {
    std::uint32_t score = 0;
    std::uint32_t maxCombo = 0;
    std::uint32_t judgedNotes = 0;
    bool cleared = false;
};

struct ResultData final {
    content::SongMetadata song;
    GameplaySummary summary;
};

} // namespace pumpdx::session
