#include "game/scenes/gameplay/GameplayScene.hpp"

#include "framework/input/KeyCode.hpp"

namespace pumpdx::scenes {

SceneId GameplayScene::Id() const noexcept {
    return SceneId::Gameplay;
}

SceneVisual GameplayScene::Visual() const noexcept {
    return {
        .clearColor = {0.015F, 0.04F, 0.075F, 1.0F},
        .windowTitle = L"PumpDX Rebuild — Gameplay (Enter: Result, Esc: Song Select)",
    };
}

std::optional<SceneId> GameplayScene::HandleKeyReleased(const std::uint32_t virtualKey) const {
    if (virtualKey == input::kConfirm) {
        return SceneId::Result;
    }
    if (virtualKey == input::kCancel) {
        return SceneId::SongSelect;
    }

    return std::nullopt;
}

} // namespace pumpdx::scenes
