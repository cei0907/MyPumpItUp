#pragma once

#include "game/gameplay/SongClock.hpp"

#include <chrono>

namespace pumpdx::gameplay {

class DebugSongClock final : public SongClock {
public:
    DebugSongClock();

    [[nodiscard]] double Seconds() const noexcept override;

private:
    std::chrono::steady_clock::time_point startTime_;
};

} // namespace pumpdx::gameplay
