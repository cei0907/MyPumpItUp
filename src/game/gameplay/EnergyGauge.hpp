#pragma once

#include "game/gameplay/Judgement.hpp"

namespace pumpdx::gameplay {

class EnergyGauge final {
public:
    void Apply(const JudgementEvent& event) noexcept;

    [[nodiscard]] double Value() const noexcept;
    [[nodiscard]] bool IsAlive() const noexcept;

private:
    double value_ = 50.0;
};

} // namespace pumpdx::gameplay
