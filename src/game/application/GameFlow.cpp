#include "game/application/GameFlow.hpp"

#include "framework/input/KeyCode.hpp"
#include "game/gameplay/FmodSongClock.hpp"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

namespace pumpdx::game {

GameFlow::GameFlow()
    : songCatalog_(content::SongCatalog::CreateFromManifestDirectory(std::filesystem::path(PUMP_DX_SONG_CATALOG_DIRECTORY))) {
    static_cast<void>(audioPlayer_.Initialize());
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
    if (CurrentSceneId() == scenes::SceneId::SongSelect) {
        if (virtualKey == 'Z') {
            difficultySelectionActive_ ? MoveDifficultySelection(-1) : MoveSongSelection(-1);
            return;
        }
        if (virtualKey == 'C') {
            difficultySelectionActive_ ? MoveDifficultySelection(1) : MoveSongSelection(1);
            return;
        }
        if (virtualKey == 'S') {
            if (difficultySelectionActive_) {
                sceneManager_.RequestScene(scenes::SceneId::Gameplay);
            } else {
                difficultySelectionActive_ = true;
            }
            return;
        }
        if (virtualKey == 'Q' || virtualKey == 'E') {
            difficultySelectionActive_ = false;
            return;
        }
        if (virtualKey == input::kConfirm) {
            sceneManager_.RequestScene(scenes::SceneId::Gameplay);
            return;
        }
        if (virtualKey == input::kCancel && difficultySelectionActive_) {
            difficultySelectionActive_ = false;
            return;
        }
    }
    sceneManager_.HandleKeyReleased(virtualKey);
}

bool GameFlow::Update() {
    audioPlayer_.Update();
    if (activeGameplay_) {
        activeGameplay_->Update();
        if (audioPlaybackStarted_ && !audioPlayer_.IsPlaying() && audioPlayer_.PlaybackSeconds() > 0.0) {
            sceneManager_.RequestScene(scenes::SceneId::Result);
        }
    }

    const auto previousScene = sceneManager_.CurrentId();
    if (!sceneManager_.Update()) {
        return false;
    }

    const auto currentScene = sceneManager_.CurrentId();
    if (previousScene == scenes::SceneId::SongSelect && currentScene == scenes::SceneId::Gameplay) {
        difficultySelectionActive_ = false;
        BeginSelectedSession();
    } else if (previousScene == scenes::SceneId::Gameplay && currentScene == scenes::SceneId::Result) {
        FinishActiveSession();
    } else if (previousScene == scenes::SceneId::Gameplay && currentScene == scenes::SceneId::SongSelect) {
        CancelActiveSession();
    }

    return true;
}

render::SongSelectOverlay GameFlow::CurrentSongSelectOverlay() const {
    const auto& selected = SelectedSong();
    const auto songCount = songCatalog_.Count();
    const auto previousIndex = selectedSongIndex_ == 0 ? songCount - 1 : selectedSongIndex_ - 1;
    const auto nextIndex = (selectedSongIndex_ + 1) % songCount;
    return {
        .title = selected.title,
        .artist = selected.artist,
        .difficultyName = selected.difficultyName,
        .previousTitle = songCount > 1 ? songCatalog_.At(previousIndex).metadata.title : std::wstring_view{},
        .nextTitle = songCount > 1 ? songCatalog_.At(nextIndex).metadata.title : std::wstring_view{},
        .difficultyLevel = selected.difficultyLevel,
        .selectedSongNumber = static_cast<std::uint32_t>(selectedSongIndex_ + 1),
        .songCount = static_cast<std::uint32_t>(songCount),
        .selectedDifficultyNumber = static_cast<std::uint32_t>(selectedDifficultyIndex_ + 1),
        .difficultyCount = static_cast<std::uint32_t>(songCatalog_.DifficultyCount(selectedSongIndex_)),
        .difficultySelectionActive = difficultySelectionActive_,
    };
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
        visual.headline = SelectedSong().title;
        visual.detail = SelectedSong().difficultyName
            + (IsUsingAudioClock() ? L"  /  FMOD" : L"  /  Debug clock");
    }
    return visual;
}

const content::SongMetadata& GameFlow::SelectedSong() const {
    return songCatalog_.At(selectedSongIndex_, selectedDifficultyIndex_).metadata;
}

const chart::Chart& GameFlow::SelectedChart() const {
    return *songCatalog_.At(selectedSongIndex_, selectedDifficultyIndex_).chart;
}

const session::PlaySession* GameFlow::ActiveSession() const noexcept {
    return activeSession_ ? &*activeSession_ : nullptr;
}

const gameplay::GameplayRuntime* GameFlow::ActiveGameplay() const noexcept {
    return activeGameplay_ ? &*activeGameplay_ : nullptr;
}

bool GameFlow::IsUsingAudioClock() const noexcept {
    return activeGameplay_.has_value() && audioPlayer_.IsPlaying();
}

const std::filesystem::path& GameFlow::ActiveStaticBgaPath() const noexcept {
    static const std::filesystem::path emptyPath;
    if (CurrentSceneId() != scenes::SceneId::Gameplay) {
        return emptyPath;
    }
    return songCatalog_.At(selectedSongIndex_, selectedDifficultyIndex_).staticBgaFilePath;
}

const std::filesystem::path& GameFlow::ActiveVideoBgaPath() const noexcept {
    static const std::filesystem::path emptyPath;
    if (CurrentSceneId() != scenes::SceneId::Gameplay) {
        return emptyPath;
    }
    return songCatalog_.At(selectedSongIndex_, selectedDifficultyIndex_).videoBgaFilePath;
}

const session::ResultData* GameFlow::LatestResult() const noexcept {
    return latestResult_ ? &*latestResult_ : nullptr;
}

void GameFlow::BeginSelectedSession() {
    activeSession_.emplace(SelectedSong(), songCatalog_.At(selectedSongIndex_, selectedDifficultyIndex_).chart);
    const auto& song = songCatalog_.At(selectedSongIndex_, selectedDifficultyIndex_);
    audioPlaybackStarted_ = false;
    if (audioPlayer_.Play(song.audioFilePath)) {
        audioPlaybackStarted_ = true;
        activeGameplay_.emplace(
            activeSession_->SelectedChart(),
            std::make_unique<gameplay::FmodSongClock>(audioPlayer_, song.audioOffsetSeconds));
    } else {
        activeGameplay_.emplace(activeSession_->SelectedChart());
    }
    latestResult_.reset();
}

void GameFlow::FinishActiveSession() {
    if (!activeSession_) {
        throw std::logic_error("Gameplay cannot finish without an active session.");
    }

    latestResult_ = activeSession_->BuildResult(activeGameplay_ ? activeGameplay_->BuildResultSummary() : session::GameplaySummary{});
    audioPlayer_.Stop();
    audioPlaybackStarted_ = false;
    activeGameplay_.reset();
    activeSession_.reset();
    sceneManager_.SetResultData(*latestResult_);
}

void GameFlow::CancelActiveSession() noexcept {
    audioPlayer_.Stop();
    audioPlaybackStarted_ = false;
    activeGameplay_.reset();
    activeSession_.reset();
}

void GameFlow::MoveSongSelection(const int direction) noexcept {
    const auto count = songCatalog_.Count();
    if (count == 0) {
        return;
    }
    if (direction < 0) {
        selectedSongIndex_ = selectedSongIndex_ == 0 ? count - 1 : selectedSongIndex_ - 1;
    } else if (direction > 0) {
        selectedSongIndex_ = (selectedSongIndex_ + 1) % count;
    }
    selectedDifficultyIndex_ = 0;
}

void GameFlow::MoveDifficultySelection(const int direction) noexcept {
    const auto count = songCatalog_.DifficultyCount(selectedSongIndex_);
    if (count == 0) {
        return;
    }
    if (direction < 0) {
        selectedDifficultyIndex_ = selectedDifficultyIndex_ == 0 ? count - 1 : selectedDifficultyIndex_ - 1;
    } else if (direction > 0) {
        selectedDifficultyIndex_ = (selectedDifficultyIndex_ + 1) % count;
    }
}

} // namespace pumpdx::game
