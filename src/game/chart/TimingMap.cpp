#include "game/chart/TimingMap.hpp"

#include <stdexcept>
#include <utility>

namespace pumpdx::chart {

TimingMap::TimingMap(std::vector<TempoSegment> segments)
    : segments_(std::move(segments)) {
    if (segments_.empty()) {
        throw std::invalid_argument("A timing map requires at least one tempo segment.");
    }
    if (!(segments_.front().startBeat == Beat::Zero())) {
        throw std::invalid_argument("The first tempo segment must start at beat zero.");
    }

    for (std::size_t index = 0; index < segments_.size(); ++index) {
        if (segments_[index].beatsPerMinute <= 0.0) {
            throw std::invalid_argument("Tempo must be greater than zero.");
        }
        if (index > 0 && !(segments_[index - 1].startBeat < segments_[index].startBeat)) {
            throw std::invalid_argument("Tempo segments must be strictly ordered by beat.");
        }
    }
}

const std::vector<TempoSegment>& TimingMap::Segments() const noexcept {
    return segments_;
}

double TimingMap::SecondsAt(const Beat& beat) const {
    if (beat < Beat::Zero()) {
        throw std::invalid_argument("Timing map does not support beats before zero.");
    }

    auto activeSegment = segments_.front();
    auto elapsedSeconds = 0.0;

    for (std::size_t index = 1; index < segments_.size(); ++index) {
        const auto& nextSegment = segments_[index];
        if (beat < nextSegment.startBeat) {
            return elapsedSeconds + (beat - activeSegment.startBeat).ToDouble() * 60.0 / activeSegment.beatsPerMinute;
        }

        elapsedSeconds += (nextSegment.startBeat - activeSegment.startBeat).ToDouble() * 60.0 / activeSegment.beatsPerMinute;
        activeSegment = nextSegment;
    }

    return elapsedSeconds + (beat - activeSegment.startBeat).ToDouble() * 60.0 / activeSegment.beatsPerMinute;
}

} // namespace pumpdx::chart
