#pragma once

#include <array>
#include <string>

namespace pumpdx::scenes {

struct SceneVisual final {
    std::array<float, 4> clearColor{};
    std::wstring windowTitle{};
    std::wstring headline{};
    std::wstring detail{};
    std::wstring instruction{};
};

} // namespace pumpdx::scenes
