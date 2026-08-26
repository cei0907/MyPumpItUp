#pragma once

namespace pumpdx::gameplay {

class SongClock {
public:
    virtual ~SongClock() = default;

    [[nodiscard]] virtual double Seconds() const noexcept = 0;
};

} // namespace pumpdx::gameplay
