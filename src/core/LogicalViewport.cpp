#include "core/LogicalViewport.hpp"

#include <algorithm>

namespace pumpdx::core {

ViewportRect LogicalViewport::FitInside(const std::uint32_t outputWidth, const std::uint32_t outputHeight) {
    if (outputWidth == 0 || outputHeight == 0) {
        return {};
    }

    const auto horizontalScale = static_cast<float>(outputWidth) / static_cast<float>(kDesignWidth);
    const auto verticalScale = static_cast<float>(outputHeight) / static_cast<float>(kDesignHeight);
    const auto scale = std::min(horizontalScale, verticalScale);

    const auto width = static_cast<float>(kDesignWidth) * scale;
    const auto height = static_cast<float>(kDesignHeight) * scale;

    return {
        .x = (static_cast<float>(outputWidth) - width) * 0.5F,
        .y = (static_cast<float>(outputHeight) - height) * 0.5F,
        .width = width,
        .height = height,
        .scale = scale,
    };
}

} // namespace pumpdx::core
