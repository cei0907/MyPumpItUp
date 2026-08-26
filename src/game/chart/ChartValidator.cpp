#include "game/chart/ChartValidator.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace pumpdx::chart {

void ChartValidator::Validate(
    const std::string_view chartId,
    [[maybe_unused]] const TimingMap& timingMap,
    const std::vector<NoteEvent>& notes) {
    if (chartId.empty()) {
        throw std::invalid_argument("A chart requires a non-empty identifier.");
    }
    if (notes.empty()) {
        throw std::invalid_argument("A chart requires at least one note event.");
    }

    for (const auto& event : notes) {
        if (!IsValidPanelLane(EventLane(event))) {
            throw std::invalid_argument("A chart contains an invalid panel lane.");
        }
        if (EventStartBeat(event) < Beat::Zero()) {
            throw std::invalid_argument("A chart cannot contain notes before beat zero.");
        }

        if (const auto* hold = std::get_if<HoldNote>(&event); hold != nullptr) {
            if (!(hold->startBeat < hold->endBeat)) {
                throw std::invalid_argument("A hold note must end after it starts.");
            }
            if (!hold->tickPolicy.IsValid()) {
                throw std::invalid_argument("A hold note requires a valid tick policy.");
            }
        }
    }

    std::array<std::vector<const HoldNote*>, 5> holdsByLane;
    for (const auto& event : notes) {
        if (const auto* hold = std::get_if<HoldNote>(&event); hold != nullptr) {
            holdsByLane[static_cast<std::size_t>(hold->lane)].push_back(hold);
        }
    }
    for (auto& laneHolds : holdsByLane) {
        std::sort(laneHolds.begin(), laneHolds.end(), [](const HoldNote* left, const HoldNote* right) {
            return left->startBeat < right->startBeat;
        });
        for (std::size_t index = 1; index < laneHolds.size(); ++index) {
            if (laneHolds[index]->startBeat < laneHolds[index - 1]->endBeat) {
                throw std::invalid_argument("Hold notes cannot overlap on the same panel lane.");
            }
        }
    }
}

} // namespace pumpdx::chart
