#pragma once

#include "framework/audio/FmodAudioPlayer.hpp"
#include "framework/render/SceneOverlayRenderer.hpp"
#include "game/content/SongCatalog.hpp"
#include "game/gameplay/GameplayRuntime.hpp"
#include "game/scenes/SceneManager.hpp"
#include "game/session/PlaySession.hpp"

#include <cstddef>
#include <cstdint>
#include <array>
#include <filesystem>
#include <optional>

namespace pumpdx::game {

class GameFlow final {
public:
    GameFlow();

    void HandleKeyPressed(std::uint32_t virtualKey);
    void HandleKeyReleased(std::uint32_t virtualKey);
    [[nodiscard]] bool Update();

    [[nodiscard]] scenes::SceneId CurrentSceneId() const noexcept;
    [[nodiscard]] scenes::SceneVisual CurrentSceneVisual() const;
    [[nodiscard]] render::SongSelectOverlay CurrentSongSelectOverlay() const;
    [[nodiscard]] const content::SongMetadata& SelectedSong() const;
    [[nodiscard]] const chart::Chart& SelectedChart() const;
    [[nodiscard]] const session::PlaySession* ActiveSession() const noexcept;
    [[nodiscard]] const gameplay::GameplayRuntime* ActiveGameplay() const noexcept;
    [[nodiscard]] bool IsUsingAudioClock() const noexcept;
    [[nodiscard]] const std::filesystem::path& ActiveStaticBgaPath() const noexcept;
    [[nodiscard]] const std::filesystem::path& ActiveVideoBgaPath() const noexcept;
    [[nodiscard]] const session::ResultData* LatestResult() const noexcept;

private:
    void BeginSelectedSession();
    void FinishActiveSession();
    void CancelActiveSession() noexcept;
    void MoveSongSelection(int direction) noexcept;
    void MoveDifficultySelection(int direction) noexcept;
    [[nodiscard]] bool TryApplySpeedCommand(std::uint32_t virtualKey) noexcept;

    content::SongCatalog songCatalog_;
    scenes::SceneManager sceneManager_;
    audio::FmodAudioPlayer audioPlayer_;
    bool audioPlaybackStarted_ = false;
    std::size_t selectedSongIndex_ = 0;
    std::size_t selectedDifficultyIndex_ = 0;
    std::array<std::uint32_t, 5> speedCommandInput_{};
    std::size_t speedCommandLength_ = 0;
    float scrollSpeed_ = 1.0F;
    bool difficultySelectionActive_ = false;
    std::optional<session::PlaySession> activeSession_;
    std::optional<gameplay::GameplayRuntime> activeGameplay_;
    std::optional<session::ResultData> latestResult_;
};

} // namespace pumpdx::game
