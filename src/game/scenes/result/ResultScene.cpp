#include "game/scenes/result/ResultScene.hpp"

#include "framework/input/KeyCode.hpp"

#include <utility>

namespace pumpdx::scenes {

ResultScene::ResultScene(std::optional<session::ResultData> result)
    : result_(std::move(result)) {
}

SceneId ResultScene::Id() const noexcept {
    return SceneId::Result;
}

SceneVisual ResultScene::Visual() const noexcept {
    const auto title = result_ ? L"PumpDX Rebuild — Result: " + result_->song.title
                               : L"PumpDX Rebuild — Result";
    const auto detail = result_ ? result_->song.title + L"  /  Score " + std::to_wstring(result_->summary.score)
                                : L"No result data";
    return {
        .clearColor = {0.09F, 0.07F, 0.015F, 1.0F},
        .windowTitle = title + L" (Enter/Esc: Song Select)",
        .headline = L"RESULT",
        .detail = detail,
        .instruction = L"Enter or Esc: back to song select",
    };
}

std::optional<SceneId> ResultScene::HandleKeyReleased(const std::uint32_t virtualKey) const {
    if (virtualKey == input::kConfirm || virtualKey == input::kCancel) {
        return SceneId::SongSelect;
    }

    return std::nullopt;
}

const session::ResultData* ResultScene::Data() const noexcept {
    return result_ ? &*result_ : nullptr;
}

} // namespace pumpdx::scenes
