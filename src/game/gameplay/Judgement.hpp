#pragma once

#include "game/chart/PanelLane.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace pumpdx::gameplay {

enum class Judgement : unsigned char {
    Perfect,
    Great,
    Good,
    Bad,
    Miss,
};

struct JudgementWindows final {
    double perfectSeconds = 0.030;
    double greatSeconds = 0.060;
    double goodSeconds = 0.100;
    double badSeconds = 0.150;

    [[nodiscard]] bool IsValid() const noexcept;
};

struct JudgableNote final {
    chart::PanelLane lane = chart::PanelLane::Center;
    double timeSeconds = 0.0;
};

struct JudgementEvent final {
    std::size_t noteIndex = 0;
    chart::PanelLane lane = chart::PanelLane::Center;
    Judgement judgement = Judgement::Miss;
    double timingErrorSeconds = 0.0;
};

[[nodiscard]] std::wstring_view JudgementLabel(Judgement judgement) noexcept;

class JudgementEngine final {
public:
    explicit JudgementEngine(std::vector<JudgableNote> notes, JudgementWindows windows = {});

    [[nodiscard]] std::optional<JudgementEvent> TryJudge(chart::PanelLane lane, double inputTimeSeconds);
    [[nodiscard]] std::vector<JudgementEvent> CollectMisses(double currentTimeSeconds);
    [[nodiscard]] bool IsResolved(std::size_t noteIndex) const noexcept;

private:
    [[nodiscard]] Judgement Classify(double absoluteErrorSeconds) const noexcept;

    std::vector<JudgableNote> notes_;
    std::vector<bool> resolved_;
    JudgementWindows windows_;
};

} // namespace pumpdx::gameplay
