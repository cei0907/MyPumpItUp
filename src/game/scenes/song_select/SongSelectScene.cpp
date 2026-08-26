#include "game/scenes/song_select/SongSelectScene.hpp"

#include "framework/input/KeyCode.hpp"

namespace pumpdx::scenes {

SceneId SongSelectScene::Id() const noexcept {
    return SceneId::SongSelect;
}

SceneVisual SongSelectScene::Visual() const noexcept {
    return {
        .clearColor = {0.11F, 0.035F, 0.19F, 1.0F},
        .windowTitle = L"PumpDX Rebuild — Song Select (Enter: Play, Esc: Menu)",
    };
}

std::optional<SceneId> SongSelectScene::HandleKeyReleased(const std::uint32_t virtualKey) const {
    if (virtualKey == input::kConfirm) {
        return SceneId::Gameplay;
    }
    if (virtualKey == input::kCancel) {
        return SceneId::MainMenu;
    }

    return std::nullopt;
}

} // namespace pumpdx::scenes
