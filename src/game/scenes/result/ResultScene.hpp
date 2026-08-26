#pragma once

#include "game/scenes/Scene.hpp"

namespace pumpdx::scenes {

class ResultScene final : public Scene {
public:
    [[nodiscard]] SceneId Id() const noexcept override;
    [[nodiscard]] SceneVisual Visual() const noexcept override;
    [[nodiscard]] std::optional<SceneId> HandleKeyReleased(std::uint32_t virtualKey) const override;
};

} // namespace pumpdx::scenes
