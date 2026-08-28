#include "game/content/SongManifest.hpp"

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

void TestManifestSeparatesDisplayMetadataFromAudioFileName() {
    const auto manifest = pumpdx::content::SongManifest::Load(PUMP_DX_TEST_SONG_MANIFEST);

    Expect(manifest.metadata.id == "legacy-new-song-to-god", "Manifest must keep a stable song id.");
    Expect(manifest.metadata.title == L"New Song To God", "Display title must come from the manifest.");
    Expect(manifest.audioFilePath.filename() == "NewSongToGod.mp3", "Audio file names must remain independent from display titles.");
    Expect(manifest.chartFilePath.filename() == "NewSongToGod_10.stp", "Chart file paths must also come from the manifest.");
    Expect(manifest.holdOverlayFilePath.filename() == "new-song-to-god-hold-playtest.pdxchart", "Hold overlays must remain explicit manifest data.");
    Expect(manifest.staticBgaFilePath.filename() == "legacy-static-fallback.png", "Static BGA paths must stay optional manifest data.");
    Expect(manifest.videoBgaFilePath.empty(), "A missing video BGA path must preserve static BGA fallback behaviour.");
    Expect(manifest.legacyStartPosition == 1052, "Legacy visual lead-in must remain explicit metadata.");
    Expect(manifest.audioOffsetSeconds == 0.0, "Audio offset must parse as seconds.");
}

} // namespace

int main() {
    TestManifestSeparatesDisplayMetadataFromAudioFileName();

    std::cout << "Song manifest tests passed.\n";
    return EXIT_SUCCESS;
}
