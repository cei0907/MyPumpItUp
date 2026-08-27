#include "game/scenes/SceneManager.hpp"

#include "game/scenes/gameplay/GameplayScene.hpp"
#include "game/scenes/main_menu/MainMenuScene.hpp"
#include "game/scenes/result/ResultScene.hpp"
#include "game/scenes/song_select/SongSelectScene.hpp"

#include <stdexcept>
#include <utility>

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

void SceneManager::RequestScene(const SceneId sceneId) noexcept {
    if (!requestedScene_.has_value()) {
        requestedScene_ = sceneId;
    }
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

void SceneManager::SetResultData(session::ResultData result) {
    if (CurrentId() != SceneId::Result) {
        throw std::logic_error("Result data can only be applied to ResultScene.");
    }

    currentScene_ = std::make_unique<ResultScene>(std::move(result));
}

SceneId SceneManager::CurrentId() const noexcept {
    return currentScene_->Id();
}

SceneVisual SceneManager::CurrentVisual() const {
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
