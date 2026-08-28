#include "framework/input/KeyCode.hpp"
#include "game/application/GameFlow.hpp"

#include <cstdlib>
#include <iostream>

namespace {

using pumpdx::input::kCancel;
using pumpdx::input::kConfirm;
using pumpdx::scenes::SceneId;

void Expect(const bool condition, const char* message) {
    if (condition) {
        return;
    }

    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

void Confirm(pumpdx::game::GameFlow& gameFlow, const SceneId expectedScene, const char* message) {
    gameFlow.HandleKeyReleased(kConfirm);
    Expect(gameFlow.Update(), "Expected a scene transition.");
    Expect(gameFlow.CurrentSceneId() == expectedScene, message);
}

void TestCompletedSessionProducesIndependentResult() {
    pumpdx::game::GameFlow gameFlow;

    Confirm(gameFlow, SceneId::SongSelect, "Main menu must enter song select.");
    const auto selectedSongId = gameFlow.SelectedSong().id;
    Confirm(gameFlow, SceneId::Gameplay, "Song select must begin gameplay.");

    const auto* session = gameFlow.ActiveSession();
    Expect(session != nullptr, "Gameplay must own an active play session.");
    Expect(session->SelectedSong().id == selectedSongId, "Play session must receive the selected song value.");
    Expect(!session->SelectedChart().Notes().empty(), "Play session must receive a non-empty chart.");
    Expect(!gameFlow.SelectedChart().Id().empty(), "Game flow must expose the selected chart.");

    Confirm(gameFlow, SceneId::Result, "Gameplay completion must enter result.");
    Expect(gameFlow.ActiveSession() == nullptr, "Result scene must not retain the live play session.");

    const auto* result = gameFlow.LatestResult();
    Expect(result != nullptr, "Gameplay completion must create result data.");
    Expect(result->song.id == selectedSongId, "Result data must preserve the selected song value.");
    Expect(result->summary.score == 0, "The State 03 placeholder summary must be deterministic.");
    Expect(result->summary.cleared, "A session with untouched starting energy must be clearable.");
}

void TestCancelledSessionDoesNotProduceResult() {
    pumpdx::game::GameFlow gameFlow;

    Confirm(gameFlow, SceneId::SongSelect, "Expected song select before cancelling gameplay.");
    Confirm(gameFlow, SceneId::Gameplay, "Expected gameplay before cancelling gameplay.");
    gameFlow.HandleKeyReleased(kCancel);
    Expect(gameFlow.Update(), "Escape in gameplay must transition to song select.");

    Expect(gameFlow.CurrentSceneId() == SceneId::SongSelect, "Cancelled gameplay must return to song select.");
    Expect(gameFlow.ActiveSession() == nullptr, "Cancelled gameplay must release its session.");
    Expect(gameFlow.LatestResult() == nullptr, "Cancelled gameplay must not create a result.");
}

void TestLegacySongSelectNavigationAndDifficultyChoice() {
    pumpdx::game::GameFlow gameFlow;

    Confirm(gameFlow, SceneId::SongSelect, "Expected song select before navigation test.");
    const auto initial = gameFlow.CurrentSongSelectOverlay();
    Expect(initial.songCount == 4, "The restored local catalog must expose four legacy menu songs.");
    Expect(initial.title == L"Africa", "Legacy menu order must begin with Africa.");

    gameFlow.HandleKeyReleased('C');
    const auto flyingDock = gameFlow.CurrentSongSelectOverlay();
    Expect(flyingDock.title == L"Flying Dock", "SE panel input must move to the next song.");
    Expect(flyingDock.difficultyCount == 2, "Flying Dock must expose its two legacy difficulties.");

    gameFlow.HandleKeyReleased('S');
    Expect(gameFlow.CurrentSongSelectOverlay().difficultySelectionActive,
        "Center panel input must enter difficulty-selection mode.");
    gameFlow.HandleKeyReleased('C');
    const auto difficultyFive = gameFlow.CurrentSongSelectOverlay();
    Expect(difficultyFive.difficultyLevel == 5, "SE panel input must move through selected-song difficulties.");

    gameFlow.HandleKeyReleased('S');
    Expect(gameFlow.Update(), "Confirming a selected difficulty must start gameplay.");
    Expect(gameFlow.CurrentSceneId() == SceneId::Gameplay, "Selected legacy difficulty must enter gameplay.");
    Expect(gameFlow.SelectedSong().difficultyLevel == 5, "Gameplay must receive the chosen difficulty.");
}

} // namespace

int main() {
    TestCompletedSessionProducesIndependentResult();
    TestCancelledSessionDoesNotProduceResult();
    TestLegacySongSelectNavigationAndDifficultyChoice();

    std::cout << "Game flow tests passed.\n";
    return EXIT_SUCCESS;
}
