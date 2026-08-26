#pragma once

#include "game/scenes/SceneId.hpp"
#include "game/scenes/SceneVisual.hpp"

#include <cstdint>
#include <optional>

namespace pumpdx::scenes {

class Scene {
public:
    virtual ~Scene() = default;

    [[nodiscard]] virtual SceneId Id() const noexcept = 0;
    [[nodiscard]] virtual SceneVisual Visual() const = 0;
    [[nodiscard]] virtual std::optional<SceneId> HandleKeyReleased(std::uint32_t virtualKey) const = 0;
};

} // namespace pumpdx::scenes
