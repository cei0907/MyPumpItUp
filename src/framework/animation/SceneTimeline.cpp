#include "framework/animation/SceneTimeline.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace pumpdx::animation {

namespace {

[[nodiscard]] float SmoothStep(const float value) noexcept {
    const auto clamped = (std::clamp)(value, 0.0F, 1.0F);
    return clamped * clamped * (3.0F - 2.0F * clamped);
}

[[nodiscard]] float Reveal(const double elapsedSeconds, const double delaySeconds) noexcept {
    return SmoothStep(static_cast<float>((elapsedSeconds - delaySeconds) / 0.28));
}

} // namespace

void SceneTimeline::Restart(const double uiTimeSeconds) noexcept {
    startedAtSeconds_ = uiTimeSeconds;
}

SceneTimelineSample SceneTimeline::Sample(const double uiTimeSeconds) const noexcept {
    const auto elapsedSeconds = (std::max)(uiTimeSeconds - startedAtSeconds_, 0.0);
    const auto phase = static_cast<float>(elapsedSeconds * std::numbers::pi * 1.5);
    return {
        .entrance = Reveal(elapsedSeconds, 0.0),
        .loopPulse = 0.5F + 0.5F * std::sin(phase),
        .detailReveal = Reveal(elapsedSeconds, 0.12),
        .instructionReveal = Reveal(elapsedSeconds, 0.28),
    };
}

} // namespace pumpdx::animation
