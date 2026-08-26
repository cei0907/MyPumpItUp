#include "game/gameplay/DebugSongClock.hpp"

namespace pumpdx::gameplay {

DebugSongClock::DebugSongClock()
    : startTime_(std::chrono::steady_clock::now()) {
}

double DebugSongClock::Seconds() const noexcept {
    const auto elapsed = std::chrono::steady_clock::now() - startTime_;
    return std::chrono::duration<double>(elapsed).count();
}

} // namespace pumpdx::gameplay
