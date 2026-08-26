#pragma once

#include "game/scenes/Scene.hpp"

#include <cstdint>
#include <memory>
#include <optional>

namespace pumpdx::scenes {

class SceneManager final {
public:
    SceneManager();

    void HandleKeyReleased(std::uint32_t virtualKey);
    [[nodiscard]] bool Update();

    [[nodiscard]] SceneId CurrentId() const noexcept;
    [[nodiscard]] SceneVisual CurrentVisual() const noexcept;

private:
    [[nodiscard]] static std::unique_ptr<Scene> CreateScene(SceneId id);

    std::unique_ptr<Scene> currentScene_;
    std::optional<SceneId> requestedScene_;
};

} // namespace pumpdx::scenes
