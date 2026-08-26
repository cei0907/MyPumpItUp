#pragma once

#include "game/scenes/Scene.hpp"
#include "game/session/ResultData.hpp"

#include <optional>

namespace pumpdx::scenes {

class ResultScene final : public Scene {
public:
    explicit ResultScene(std::optional<session::ResultData> result = std::nullopt);

    [[nodiscard]] SceneId Id() const noexcept override;
    [[nodiscard]] SceneVisual Visual() const noexcept override;
    [[nodiscard]] std::optional<SceneId> HandleKeyReleased(std::uint32_t virtualKey) const override;
    [[nodiscard]] const session::ResultData* Data() const noexcept;

private:
    std::optional<session::ResultData> result_;
};

} // namespace pumpdx::scenes
