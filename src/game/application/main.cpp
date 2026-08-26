#include "game/application/GameApplication.hpp"

#include <Windows.h>

int APIENTRY wWinMain(
    const HINSTANCE instanceHandle,
    [[maybe_unused]] const HINSTANCE previousInstanceHandle,
    [[maybe_unused]] const PWSTR commandLine,
    [[maybe_unused]] const int showCommand) {
    pumpdx::game::GameApplication application(instanceHandle);
    return application.Run();
}
