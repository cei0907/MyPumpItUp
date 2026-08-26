#include "game/scenes/main_menu/MainMenuScene.hpp"

#include "framework/input/KeyCode.hpp"

namespace pumpdx::scenes {

SceneId MainMenuScene::Id() const noexcept {
    return SceneId::MainMenu;
}

SceneVisual MainMenuScene::Visual() const noexcept {
    return {
        .clearColor = {0.025F, 0.095F, 0.205F, 1.0F},
        .windowTitle = L"PumpDX Rebuild — Main Menu (Enter: Song Select)",
        .headline = L"PUMP DX",
        .detail = L"Rebuild Foundation",
        .instruction = L"Press Enter to select a song",
    };
}

std::optional<SceneId> MainMenuScene::HandleKeyReleased(const std::uint32_t virtualKey) const {
    if (virtualKey == input::kConfirm) {
        return SceneId::SongSelect;
    }

    return std::nullopt;
}

} // namespace pumpdx::scenes
