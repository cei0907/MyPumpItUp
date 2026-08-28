#include "game/chart/Chart.hpp"

#include "game/chart/ChartValidator.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace pumpdx::chart {

Chart::Chart(std::string id, TimingMap timingMap, std::vector<NoteEvent> notes, const double delaySeconds)
    : id_(std::move(id))
    , timingMap_(std::move(timingMap))
    , notes_(std::move(notes))
    , delaySeconds_(delaySeconds) {
    if (!std::isfinite(delaySeconds_) || delaySeconds_ < 0.0) {
        throw std::invalid_argument("A chart delay must be a finite non-negative duration.");
    }
    ChartValidator::Validate(id_, timingMap_, notes_);

    std::sort(notes_.begin(), notes_.end(), [](const NoteEvent& left, const NoteEvent& right) {
        const auto& leftBeat = EventStartBeat(left);
        const auto& rightBeat = EventStartBeat(right);
        if (leftBeat != rightBeat) {
            return leftBeat < rightBeat;
        }
        return static_cast<unsigned int>(EventLane(left)) < static_cast<unsigned int>(EventLane(right));
    });
}

const std::string& Chart::Id() const noexcept {
    return id_;
}

const TimingMap& Chart::Timing() const noexcept {
    return timingMap_;
}

const std::vector<NoteEvent>& Chart::Notes() const noexcept {
    return notes_;
}

double Chart::DelaySeconds() const noexcept {
    return delaySeconds_;
}

} // namespace pumpdx::chart
