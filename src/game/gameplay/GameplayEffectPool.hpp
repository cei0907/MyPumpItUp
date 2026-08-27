#pragma once

#include "framework/render/GameplayFeedback.hpp"
#include "game/gameplay/Judgement.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pumpdx::gameplay {

// Fixed-capacity, allocation-free pool for short visual reactions to judgement events.
class GameplayEffectPool final {
public:
    void Trigger(const JudgementEvent& event, double songTimeSeconds) noexcept;
    [[nodiscard]] render::GameplayFeedback Sample(double songTimeSeconds) const noexcept;

private:
    struct Effect final {
        chart::PanelLane lane = chart::PanelLane::Center;
        Judgement judgement = Judgement::Miss;
        double startedAtSeconds = 0.0;
        std::uint64_t sequence = 0;
        bool active = false;
    };

    [[nodiscard]] static float Strength(Judgement judgement) noexcept;
    [[nodiscard]] static bool IsExpired(const Effect& effect, double songTimeSeconds) noexcept;

    static constexpr double kLifetimeSeconds = 0.30;
    std::array<Effect, 32> effects_{};
    std::uint64_t nextSequence_ = 1;
};

} // namespace pumpdx::gameplay
