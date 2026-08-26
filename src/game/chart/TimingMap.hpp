#pragma once

#include "game/chart/Beat.hpp"

#include <vector>

namespace pumpdx::chart {

struct TempoSegment final {
    Beat startBeat{};
    double beatsPerMinute = 0.0;
};

class TimingMap final {
public:
    explicit TimingMap(std::vector<TempoSegment> segments);

    [[nodiscard]] double SecondsAt(const Beat& beat) const;

private:
    std::vector<TempoSegment> segments_;
};

} // namespace pumpdx::chart
