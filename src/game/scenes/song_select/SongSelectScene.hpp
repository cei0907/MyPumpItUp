#pragma once

#include "game/scenes/Scene.hpp"

namespace pumpdx::scenes {

class SongSelectScene final : public Scene {
public:
    [[nodiscard]] SceneId Id() const noexcept override;
    [[nodiscard]] SceneVisual Visual() const override;
    [[nodiscard]] std::optional<SceneId> HandleKeyReleased(std::uint32_t virtualKey) const override;
};

} // namespace pumpdx::scenes
