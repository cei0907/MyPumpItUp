#include "framework/input/KeyCode.hpp"
#include "game/scenes/SceneManager.hpp"

#include <cstdlib>
#include <iostream>

namespace {

void Expect(const bool condition, const char* message) {
    if (condition) {
        return;
    }

    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

void ConfirmTransition(
    pumpdx::scenes::SceneManager& sceneManager,
    const pumpdx::scenes::SceneId expectedScene,
    const char* message) {
    sceneManager.HandleKeyReleased(pumpdx::input::kConfirm);
    Expect(sceneManager.Update(), "Expected a pending scene transition.");
    Expect(sceneManager.CurrentId() == expectedScene, message);
}

void TestHappyPath() {
    pumpdx::scenes::SceneManager sceneManager;

    Expect(sceneManager.CurrentId() == pumpdx::scenes::SceneId::MainMenu, "Main menu must be the initial scene.");
    ConfirmTransition(sceneManager, pumpdx::scenes::SceneId::SongSelect, "Enter on main menu must open song select.");
    ConfirmTransition(sceneManager, pumpdx::scenes::SceneId::Gameplay, "Enter on song select must start gameplay.");
    ConfirmTransition(sceneManager, pumpdx::scenes::SceneId::Result, "Enter on gameplay must open result.");
    ConfirmTransition(sceneManager, pumpdx::scenes::SceneId::SongSelect, "Enter on result must return to song select.");
}

void TestCancelTransitions() {
    pumpdx::scenes::SceneManager sceneManager;
    ConfirmTransition(sceneManager, pumpdx::scenes::SceneId::SongSelect, "Expected song select before testing cancel.");

    sceneManager.HandleKeyReleased(pumpdx::input::kCancel);
    Expect(sceneManager.Update(), "Escape from song select must request a transition.");
    Expect(sceneManager.CurrentId() == pumpdx::scenes::SceneId::MainMenu, "Escape on song select must return to main menu.");
}

void TestUnsupportedKeyDoesNotTransition() {
    pumpdx::scenes::SceneManager sceneManager;
    sceneManager.HandleKeyReleased('X');

    Expect(!sceneManager.Update(), "An unsupported key must not change the scene.");
    Expect(sceneManager.CurrentId() == pumpdx::scenes::SceneId::MainMenu, "Unsupported key changed the current scene.");
}

} // namespace

int main() {
    TestHappyPath();
    TestCancelTransitions();
    TestUnsupportedKeyDoesNotTransition();

    std::cout << "SceneManager tests passed.\n";
    return EXIT_SUCCESS;
}
