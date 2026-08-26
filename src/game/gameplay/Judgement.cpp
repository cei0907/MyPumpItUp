#include "game/gameplay/Judgement.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace pumpdx::gameplay {

namespace {

constexpr double kComparisonEpsilon = 0.000000001;

} // namespace

bool JudgementWindows::IsValid() const noexcept {
    return perfectSeconds >= 0.0
        && perfectSeconds <= greatSeconds
        && greatSeconds <= goodSeconds
        && goodSeconds <= badSeconds;
}

std::wstring_view JudgementLabel(const Judgement judgement) noexcept {
    switch (judgement) {
    case Judgement::Perfect: return L"PERFECT";
    case Judgement::Great: return L"GREAT";
    case Judgement::Good: return L"GOOD";
    case Judgement::Bad: return L"BAD";
    case Judgement::Miss: return L"MISS";
    }

    return L"";
}

JudgementEngine::JudgementEngine(std::vector<JudgableNote> notes, const JudgementWindows windows)
    : notes_(std::move(notes))
    , resolved_(notes_.size(), false)
    , windows_(windows) {
    if (!windows_.IsValid()) {
        throw std::invalid_argument("Judgement windows must be ordered and non-negative.");
    }
}

std::optional<JudgementEvent> JudgementEngine::TryJudge(
    const chart::PanelLane lane,
    const double inputTimeSeconds) {
    for (std::size_t index = 0; index < notes_.size(); ++index) {
        const auto& note = notes_[index];
        if (resolved_[index] || note.lane != lane) {
            continue;
        }

        const auto timingError = inputTimeSeconds - note.timeSeconds;
        const auto absoluteError = std::abs(timingError);
        if (absoluteError > windows_.badSeconds + kComparisonEpsilon) {
            return std::nullopt;
        }

        resolved_[index] = true;
        return JudgementEvent{
            .noteIndex = index,
            .lane = lane,
            .judgement = Classify(absoluteError),
            .timingErrorSeconds = timingError,
        };
    }

    return std::nullopt;
}

std::vector<JudgementEvent> JudgementEngine::CollectMisses(const double currentTimeSeconds) {
    std::vector<JudgementEvent> misses;
    for (std::size_t index = 0; index < notes_.size(); ++index) {
        const auto& note = notes_[index];
        if (resolved_[index] || currentTimeSeconds <= note.timeSeconds + windows_.badSeconds + kComparisonEpsilon) {
            continue;
        }

        resolved_[index] = true;
        misses.push_back({
            .noteIndex = index,
            .lane = note.lane,
            .judgement = Judgement::Miss,
            .timingErrorSeconds = currentTimeSeconds - note.timeSeconds,
        });
    }

    return misses;
}

bool JudgementEngine::IsResolved(const std::size_t noteIndex) const noexcept {
    return noteIndex < resolved_.size() && resolved_[noteIndex];
}

Judgement JudgementEngine::Classify(const double absoluteErrorSeconds) const noexcept {
    if (absoluteErrorSeconds <= windows_.perfectSeconds + kComparisonEpsilon) {
        return Judgement::Perfect;
    }
    if (absoluteErrorSeconds <= windows_.greatSeconds + kComparisonEpsilon) {
        return Judgement::Great;
    }
    if (absoluteErrorSeconds <= windows_.goodSeconds + kComparisonEpsilon) {
        return Judgement::Good;
    }
    return Judgement::Bad;
}

} // namespace pumpdx::gameplay
