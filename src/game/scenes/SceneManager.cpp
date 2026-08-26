#include "game/scenes/SceneManager.hpp"

#include "game/scenes/gameplay/GameplayScene.hpp"
#include "game/scenes/main_menu/MainMenuScene.hpp"
#include "game/scenes/result/ResultScene.hpp"
#include "game/scenes/song_select/SongSelectScene.hpp"

#include <stdexcept>

namespace pumpdx::scenes {

SceneManager::SceneManager()
    : currentScene_(CreateScene(SceneId::MainMenu)) {
}

void SceneManager::HandleKeyReleased(const std::uint32_t virtualKey) {
    if (requestedScene_.has_value()) {
        return;
    }

    requestedScene_ = currentScene_->HandleKeyReleased(virtualKey);
}

bool SceneManager::Update() {
    if (!requestedScene_.has_value()) {
        return false;
    }

    const auto nextScene = *requestedScene_;
    requestedScene_.reset();

    if (nextScene == currentScene_->Id()) {
        return false;
    }

    currentScene_ = CreateScene(nextScene);
    return true;
}

SceneId SceneManager::CurrentId() const noexcept {
    return currentScene_->Id();
}

SceneVisual SceneManager::CurrentVisual() const noexcept {
    return currentScene_->Visual();
}

std::unique_ptr<Scene> SceneManager::CreateScene(const SceneId id) {
    switch (id) {
    case SceneId::MainMenu:
        return std::make_unique<MainMenuScene>();
    case SceneId::SongSelect:
        return std::make_unique<SongSelectScene>();
    case SceneId::Gameplay:
        return std::make_unique<GameplayScene>();
    case SceneId::Result:
        return std::make_unique<ResultScene>();
    }

    throw std::logic_error("Unknown scene identifier.");
}

} // namespace pumpdx::scenes
