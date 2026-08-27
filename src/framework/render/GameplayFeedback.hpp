#pragma once

#include <array>

namespace pumpdx::render {

// A renderer-facing snapshot. Values are derived from short-lived gameplay events,
// so the renderer never owns judgement or scoring state.
struct GameplayFeedback final {
    std::array<float, 5> receptorImpact{};
    float judgementBurst = 0.0F;
    float comboScale = 1.0F;
    float gaugeImpact = 0.0F;
};

} // namespace pumpdx::render
