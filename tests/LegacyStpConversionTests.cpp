#include "game/chart/LegacyStpConverter.hpp"
#include "game/chart/NativeChartLoader.hpp"
#include "game/chart/NativeChartWriter.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <variant>

namespace {

void Expect(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestConversionPreservesLegacyTapsAndMergesHolds() {
    const auto converted = pumpdx::chart::LegacyStpConverter::Convert(
        PUMP_DX_TEST_LEGACY_STP,
        "converted-legacy-test",
        263,
        std::filesystem::path(PUMP_DX_TEST_NATIVE_CHART));
    Expect(converted.Notes().size() == 10, "Conversion must preserve six legacy taps and merge four native events.");

    const auto temporaryPath = std::filesystem::temp_directory_path() / "pumpdx-legacy-conversion-test.pdxchart";
    pumpdx::chart::NativeChartWriter::Save(converted, temporaryPath);
    const auto reloaded = pumpdx::chart::NativeChartLoader::Load(temporaryPath);
    std::filesystem::remove(temporaryPath);

    Expect(reloaded.Id() == converted.Id(), "Converted chart id must round-trip through the native writer.");
    Expect(reloaded.Notes().size() == converted.Notes().size(), "Native conversion must round-trip every event.");
    for (std::size_t index = 0; index < converted.Notes().size(); ++index) {
        const auto& expected = converted.Notes()[index];
        const auto& actual = reloaded.Notes()[index];
        Expect(pumpdx::chart::EventLane(actual) == pumpdx::chart::EventLane(expected), "Converted event lane changed after round-trip.");
        Expect(pumpdx::chart::EventStartBeat(actual) == pumpdx::chart::EventStartBeat(expected), "Converted event beat changed after round-trip.");
        Expect(actual.index() == expected.index(), "Converted event type changed after round-trip.");
        if (const auto* expectedHold = std::get_if<pumpdx::chart::HoldNote>(&expected); expectedHold != nullptr) {
            const auto* actualHold = std::get_if<pumpdx::chart::HoldNote>(&actual);
            Expect(actualHold != nullptr && actualHold->endBeat == expectedHold->endBeat,
                "Converted hold end beat changed after round-trip.");
            Expect(actualHold != nullptr && actualHold->tickPolicy.tickCount == expectedHold->tickPolicy.tickCount,
                "Converted hold tick count changed after round-trip.");
        }
    }
}

} // namespace

int main() {
    TestConversionPreservesLegacyTapsAndMergesHolds();
    std::cout << "Legacy .stp conversion tests passed.\n";
    return EXIT_SUCCESS;
}
