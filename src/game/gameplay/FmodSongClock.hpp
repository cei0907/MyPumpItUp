#pragma once

#include "game/gameplay/SongClock.hpp"

namespace pumpdx::audio {
class FmodAudioPlayer;
}

namespace pumpdx::gameplay {

class FmodSongClock final : public SongClock {
public:
    FmodSongClock(const audio::FmodAudioPlayer& player, double offsetSeconds) noexcept;

    [[nodiscard]] double Seconds() const noexcept override;

private:
    const audio::FmodAudioPlayer& player_;
    double offsetSeconds_ = 0.0;
};

} // namespace pumpdx::gameplay
