#pragma once

#include "game/scenes/Scene.hpp"
#include "game/session/ResultData.hpp"

#include <cstdint>
#include <memory>
#include <optional>

namespace pumpdx::scenes {

class SceneManager final {
public:
    SceneManager();

    void HandleKeyReleased(std::uint32_t virtualKey);
    void RequestScene(SceneId sceneId) noexcept;
    [[nodiscard]] bool Update();
    void SetResultData(session::ResultData result);

    [[nodiscard]] SceneId CurrentId() const noexcept;
    [[nodiscard]] SceneVisual CurrentVisual() const;

private:
    [[nodiscard]] static std::unique_ptr<Scene> CreateScene(SceneId id);

    std::unique_ptr<Scene> currentScene_;
    std::optional<SceneId> requestedScene_;
};

} // namespace pumpdx::scenes
