#include "game/scenes/result/ResultScene.hpp"

#include "framework/input/KeyCode.hpp"

namespace pumpdx::scenes {

SceneId ResultScene::Id() const noexcept {
    return SceneId::Result;
}

SceneVisual ResultScene::Visual() const noexcept {
    return {
        .clearColor = {0.09F, 0.07F, 0.015F, 1.0F},
        .windowTitle = L"PumpDX Rebuild — Result (Enter/Esc: Song Select)",
    };
}

std::optional<SceneId> ResultScene::HandleKeyReleased(const std::uint32_t virtualKey) const {
    if (virtualKey == input::kConfirm || virtualKey == input::kCancel) {
        return SceneId::SongSelect;
    }

    return std::nullopt;
}

} // namespace pumpdx::scenes
