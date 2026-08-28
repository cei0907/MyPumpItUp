#include "framework/video/MediaFoundationBgaPlayer.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {

void Expect(const bool condition, const char* message) {
    if (condition) {
        return;
    }

    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

void TestMissingVideoLeavesTheStaticFallbackPathAvailable() {
    pumpdx::video::MediaFoundationBgaPlayer player;
    const auto missingPath = std::filesystem::path("this-file-does-not-exist.mp4");

    Expect(!player.Open(missingPath), "A missing BGA video must fail safely rather than open a decoder.");
    Expect(player.FrameAt(0.0) == nullptr, "A failed BGA video must not expose an invalid frame.");
    player.Close();
}

} // namespace

int main() {
    TestMissingVideoLeavesTheStaticFallbackPathAvailable();

    std::cout << "BGA video-player tests passed.\n";
    return EXIT_SUCCESS;
}
