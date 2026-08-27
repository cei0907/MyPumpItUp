#include "game/gameplay/EnergyGauge.hpp"

#include <algorithm>

namespace pumpdx::gameplay {

void EnergyGauge::Apply(const JudgementEvent& event) noexcept {
    double delta = 0.0;
    switch (event.judgement) {
    case Judgement::Perfect: delta = 2.5; break;
    case Judgement::Great: delta = 1.5; break;
    case Judgement::Good: delta = 0.5; break;
    case Judgement::Bad: delta = -4.0; break;
    case Judgement::Miss: delta = -7.0; break;
    }

    value_ = (std::clamp)(value_ + delta, 0.0, 100.0);
}

double EnergyGauge::Value() const noexcept {
    return value_;
}

bool EnergyGauge::IsAlive() const noexcept {
    return value_ > 0.0;
}

} // namespace pumpdx::gameplay
