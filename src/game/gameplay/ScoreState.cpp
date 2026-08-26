#include "game/gameplay/ScoreState.hpp"

#include <algorithm>

namespace pumpdx::gameplay {

namespace {

[[nodiscard]] constexpr std::uint32_t ScoreValue(const Judgement judgement) noexcept {
    switch (judgement) {
    case Judgement::Perfect: return 1000;
    case Judgement::Great: return 700;
    case Judgement::Good: return 400;
    case Judgement::Bad: return 100;
    case Judgement::Miss: return 0;
    }

    return 0;
}

} // namespace

void ScoreState::Apply(const JudgementEvent& event) noexcept {
    ++judgedNotes_;
    latestJudgement_ = event.judgement;
    score_ += ScoreValue(event.judgement);

    if (event.judgement == Judgement::Miss) {
        currentCombo_ = 0;
        return;
    }

    ++currentCombo_;
    maxCombo_ = (std::max)(maxCombo_, currentCombo_);
}

std::uint32_t ScoreState::Score() const noexcept {
    return score_;
}

std::uint32_t ScoreState::CurrentCombo() const noexcept {
    return currentCombo_;
}

std::uint32_t ScoreState::MaxCombo() const noexcept {
    return maxCombo_;
}

std::optional<Judgement> ScoreState::LatestJudgement() const noexcept {
    return latestJudgement_;
}

session::GameplaySummary ScoreState::BuildSummary() const noexcept {
    return {
        .score = score_,
        .maxCombo = maxCombo_,
        .judgedNotes = judgedNotes_,
        .cleared = false,
    };
}

} // namespace pumpdx::gameplay
