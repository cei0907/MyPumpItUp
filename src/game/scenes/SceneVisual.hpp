#pragma once

#include <array>
#include <string_view>

namespace pumpdx::scenes {

struct SceneVisual final {
    std::array<float, 4> clearColor{};
    std::wstring_view windowTitle{};
};

} // namespace pumpdx::scenes
