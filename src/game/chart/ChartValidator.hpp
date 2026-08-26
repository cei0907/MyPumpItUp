#pragma once

#include "game/chart/NoteEvent.hpp"
#include "game/chart/TimingMap.hpp"

#include <string_view>
#include <vector>

namespace pumpdx::chart {

class ChartValidator final {
public:
    static void Validate(
        std::string_view chartId,
        const TimingMap& timingMap,
        const std::vector<NoteEvent>& notes);
};

} // namespace pumpdx::chart
