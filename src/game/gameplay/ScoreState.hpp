#pragma once

#include "game/gameplay/Judgement.hpp"
#include "game/session/ResultData.hpp"

#include <cstdint>
#include <optional>

namespace pumpdx::gameplay {

class ScoreState final {
public:
    void Apply(const JudgementEvent& event) noexcept;

    [[nodiscard]] std::uint32_t Score() const noexcept;
    [[nodiscard]] std::uint32_t CurrentCombo() const noexcept;
    [[nodiscard]] std::uint32_t MaxCombo() const noexcept;
    [[nodiscard]] std::uint32_t HoldTicks() const noexcept;
    [[nodiscard]] std::optional<Judgement> LatestJudgement() const noexcept;
    [[nodiscard]] session::GameplaySummary BuildSummary() const noexcept;

private:
    std::uint32_t score_ = 0;
    std::uint32_t currentCombo_ = 0;
    std::uint32_t maxCombo_ = 0;
    std::uint32_t judgedNotes_ = 0;
    std::uint32_t holdTicks_ = 0;
    std::optional<Judgement> latestJudgement_;
};

} // namespace pumpdx::gameplay
