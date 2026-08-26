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
    Expect(session->SelectedChart().Notes().size() == 4, "Play session must receive the selected chart.");
    Expect(gameFlow.SelectedChart().Id() == "state-06-demo-foundation", "Game flow must expose the selected chart.");

    Confirm(gameFlow, SceneId::Result, "Gameplay completion must enter result.");
    Expect(gameFlow.ActiveSession() == nullptr, "Result scene must not retain the live play session.");

    const auto* result = gameFlow.LatestResult();
    Expect(result != nullptr, "Gameplay completion must create result data.");
    Expect(result->song.id == selectedSongId, "Result data must preserve the selected song value.");
    Expect(result->summary.score == 0, "The State 03 placeholder summary must be deterministic.");
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

} // namespace

int main() {
    TestCompletedSessionProducesIndependentResult();
    TestCancelledSessionDoesNotProduceResult();

    std::cout << "Game flow tests passed.\n";
    return EXIT_SUCCESS;
}
