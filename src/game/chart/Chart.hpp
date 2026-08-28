#pragma once

#include "game/chart/NoteEvent.hpp"
#include "game/chart/TimingMap.hpp"

#include <string>
#include <vector>

namespace pumpdx::chart {

class Chart final {
public:
    Chart(std::string id, TimingMap timingMap, std::vector<NoteEvent> notes, double delaySeconds = 0.0);

    [[nodiscard]] const std::string& Id() const noexcept;
    [[nodiscard]] const TimingMap& Timing() const noexcept;
    [[nodiscard]] const std::vector<NoteEvent>& Notes() const noexcept;
    [[nodiscard]] double DelaySeconds() const noexcept;

private:
    std::string id_;
    TimingMap timingMap_;
    std::vector<NoteEvent> notes_;
    double delaySeconds_ = 0.0;
};

} // namespace pumpdx::chart
