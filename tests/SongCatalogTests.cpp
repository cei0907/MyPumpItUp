#include "game/content/SongCatalog.hpp"

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

void TestLegacyTapsAndNativeHoldsMergeWithoutMutatingEitherSource() {
    const auto catalog = pumpdx::content::SongCatalog::CreateDemoCatalog(PUMP_DX_TEST_OVERLAY_MANIFEST);
    const auto& notes = catalog.At(0).chart->Notes();

    std::size_t holdCount = 0;
    for (const auto& note : notes) {
        if (std::holds_alternative<pumpdx::chart::HoldNote>(note)) {
            ++holdCount;
        }
    }

    Expect(notes.size() == 10, "Overlay catalog must keep six legacy taps and add four native events.");
    Expect(holdCount == 2, "Native overlay must add its two hold notes to the legacy tap chart.");
    Expect(catalog.At(0).chart->Id() == "legacy-overlay-test-legacy-stp-hold-overlay",
        "Merged charts must receive a distinct runtime id.");
}

} // namespace

int main() {
    TestLegacyTapsAndNativeHoldsMergeWithoutMutatingEitherSource();
    std::cout << "Song catalog tests passed.\n";
    return EXIT_SUCCESS;
}
