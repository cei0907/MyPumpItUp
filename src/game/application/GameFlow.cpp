#include "game/application/GameFlow.hpp"

#include <stdexcept>

namespace pumpdx::game {

GameFlow::GameFlow()
    : songCatalog_(content::SongCatalog::CreateDemoCatalog()) {
}

void GameFlow::HandleKeyReleased(const std::uint32_t virtualKey) {
    sceneManager_.HandleKeyReleased(virtualKey);
}

bool GameFlow::Update() {
    const auto previousScene = sceneManager_.CurrentId();
    if (!sceneManager_.Update()) {
        return false;
    }

    const auto currentScene = sceneManager_.CurrentId();
    if (previousScene == scenes::SceneId::SongSelect && currentScene == scenes::SceneId::Gameplay) {
        BeginSelectedSession();
    } else if (previousScene == scenes::SceneId::Gameplay && currentScene == scenes::SceneId::Result) {
        FinishActiveSession();
    } else if (previousScene == scenes::SceneId::Gameplay && currentScene == scenes::SceneId::SongSelect) {
        CancelActiveSession();
    }

    return true;
}

scenes::SceneId GameFlow::CurrentSceneId() const noexcept {
    return sceneManager_.CurrentId();
}

scenes::SceneVisual GameFlow::CurrentSceneVisual() const noexcept {
    return sceneManager_.CurrentVisual();
}

const content::SongMetadata& GameFlow::SelectedSong() const {
    return songCatalog_.At(selectedSongIndex_);
}

const session::PlaySession* GameFlow::ActiveSession() const noexcept {
    return activeSession_ ? &*activeSession_ : nullptr;
}

const session::ResultData* GameFlow::LatestResult() const noexcept {
    return latestResult_ ? &*latestResult_ : nullptr;
}

void GameFlow::BeginSelectedSession() {
    activeSession_.emplace(SelectedSong());
    latestResult_.reset();
}

void GameFlow::FinishActiveSession() {
    if (!activeSession_) {
        throw std::logic_error("Gameplay cannot finish without an active session.");
    }

    latestResult_ = activeSession_->BuildResult({});
    activeSession_.reset();
    sceneManager_.SetResultData(*latestResult_);
}

void GameFlow::CancelActiveSession() noexcept {
    activeSession_.reset();
}

} // namespace pumpdx::game
