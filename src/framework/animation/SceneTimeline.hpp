#pragma once

namespace pumpdx::animation {

struct SceneTimelineSample final {
    float entrance = 0.0F;
    float loopPulse = 0.0F;
    float detailReveal = 0.0F;
    float instructionReveal = 0.0F;
};

// UI-only clock. It is intentionally independent from the audio/gameplay clock.
class SceneTimeline final {
public:
    void Restart(double uiTimeSeconds) noexcept;
    [[nodiscard]] SceneTimelineSample Sample(double uiTimeSeconds) const noexcept;

private:
    double startedAtSeconds_ = 0.0;
};

} // namespace pumpdx::animation
