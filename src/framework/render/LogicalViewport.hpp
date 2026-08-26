#pragma once

#include <cstdint>

namespace pumpdx::core {

struct ViewportRect final {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
    float scale = 0.0F;
};

class LogicalViewport final {
public:
    static constexpr std::uint32_t kDesignWidth = 1280;
    static constexpr std::uint32_t kDesignHeight = 720;

    [[nodiscard]] static ViewportRect FitInside(std::uint32_t outputWidth, std::uint32_t outputHeight);
};

} // namespace pumpdx::core
