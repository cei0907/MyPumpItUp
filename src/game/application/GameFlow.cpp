#include "game/application/GameFlow.hpp"

#include "framework/input/KeyCode.hpp"

#include <stdexcept>
#include <string>

namespace pumpdx::game {

GameFlow::GameFlow()
    : songCatalog_(content::SongCatalog::CreateDemoCatalog()) {
}

void GameFlow::HandleKeyPressed(const std::uint32_t virtualKey) {
    if (!activeGameplay_) {
        return;
    }
    if (const auto panel = input::ToFivePanelInput(virtualKey); panel.has_value()) {
        activeGameplay_->SetPanelPressed(static_cast<chart::PanelLane>(*panel), true);
    }
}

void GameFlow::HandleKeyReleased(const std::uint32_t virtualKey) {
    if (activeGameplay_) {
        if (const auto panel = input::ToFivePanelInput(virtualKey); panel.has_value()) {
            activeGameplay_->SetPanelPressed(static_cast<chart::PanelLane>(*panel), false);
        }
    }
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

scenes::SceneVisual GameFlow::CurrentSceneVisual() const {
    auto visual = sceneManager_.CurrentVisual();
    if (CurrentSceneId() == scenes::SceneId::SongSelect) {
        visual.detail = SelectedSong().title + L"  /  " + SelectedSong().difficultyName
            + L"  Lv. " + std::to_wstring(SelectedSong().difficultyLevel);
    } else if (CurrentSceneId() == scenes::SceneId::Gameplay) {
        visual.detail = SelectedSong().title + L"  /  "
            + std::to_wstring(SelectedChart().Notes().size()) + L" note events  /  Debug clock";
    }
    return visual;
}

const content::SongMetadata& GameFlow::SelectedSong() const {
    return songCatalog_.At(selectedSongIndex_).metadata;
}

const chart::Chart& GameFlow::SelectedChart() const {
    return *songCatalog_.At(selectedSongIndex_).chart;
}

const session::PlaySession* GameFlow::ActiveSession() const noexcept {
    return activeSession_ ? &*activeSession_ : nullptr;
}

const gameplay::GameplayRuntime* GameFlow::ActiveGameplay() const noexcept {
    return activeGameplay_ ? &*activeGameplay_ : nullptr;
}

const session::ResultData* GameFlow::LatestResult() const noexcept {
    return latestResult_ ? &*latestResult_ : nullptr;
}

void GameFlow::BeginSelectedSession() {
    activeSession_.emplace(SelectedSong(), songCatalog_.At(selectedSongIndex_).chart);
    activeGameplay_.emplace(activeSession_->SelectedChart());
    latestResult_.reset();
}

void GameFlow::FinishActiveSession() {
    if (!activeSession_) {
        throw std::logic_error("Gameplay cannot finish without an active session.");
    }

    latestResult_ = activeSession_->BuildResult({});
    activeGameplay_.reset();
    activeSession_.reset();
    sceneManager_.SetResultData(*latestResult_);
}

void GameFlow::CancelActiveSession() noexcept {
    activeGameplay_.reset();
    activeSession_.reset();
}

} // namespace pumpdx::game
