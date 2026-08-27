#include "game/gameplay/FmodSongClock.hpp"

#include "framework/audio/FmodAudioPlayer.hpp"

#include <algorithm>

namespace pumpdx::gameplay {

FmodSongClock::FmodSongClock(const audio::FmodAudioPlayer& player, const double offsetSeconds) noexcept
    : player_(player)
    , offsetSeconds_(offsetSeconds) {
}

double FmodSongClock::Seconds() const noexcept {
    return (std::max)(0.0, player_.PlaybackSeconds() + offsetSeconds_);
}

} // namespace pumpdx::gameplay
