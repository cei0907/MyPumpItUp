#pragma once

#include "game/content/SongCatalog.hpp"
#include "game/scenes/SceneManager.hpp"
#include "game/session/PlaySession.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace pumpdx::game {

class GameFlow final {
public:
    GameFlow();

    void HandleKeyReleased(std::uint32_t virtualKey);
    [[nodiscard]] bool Update();

    [[nodiscard]] scenes::SceneId CurrentSceneId() const noexcept;
    [[nodiscard]] scenes::SceneVisual CurrentSceneVisual() const noexcept;
    [[nodiscard]] const content::SongMetadata& SelectedSong() const;
    [[nodiscard]] const session::PlaySession* ActiveSession() const noexcept;
    [[nodiscard]] const session::ResultData* LatestResult() const noexcept;

private:
    void BeginSelectedSession();
    void FinishActiveSession();
    void CancelActiveSession() noexcept;

    content::SongCatalog songCatalog_;
    scenes::SceneManager sceneManager_;
    std::size_t selectedSongIndex_ = 0;
    std::optional<session::PlaySession> activeSession_;
    std::optional<session::ResultData> latestResult_;
};

} // namespace pumpdx::game
