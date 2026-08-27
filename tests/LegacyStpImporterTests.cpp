#include "game/chart/LegacyStpImporter.hpp"

#include <cmath>
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

void TestFiveLaneSixteenthGridConvertsToTapNotes() {
    const auto chart = pumpdx::chart::LegacyStpImporter::LoadTapChart(PUMP_DX_TEST_LEGACY_STP, "legacy-test", 263);
    const auto& notes = chart.Notes();

    Expect(notes.size() == 6, "Legacy .stp importer must ignore only trailing symbols beyond the first-lane grid.");
    Expect(pumpdx::chart::EventLane(notes.front()) == pumpdx::chart::PanelLane::DownLeft, "First legacy lane must map to DownLeft.");
    Expect(std::abs(chart.Timing().SecondsAt(pumpdx::chart::Beat(5, 4)) - 0.625) < 0.000001,
        "A legacy slot must retain its sixteenth-note interval after the visual lead-in.");
    Expect(pumpdx::chart::EventStartBeat(notes.front()) == pumpdx::chart::Beat(1),
        "Legacy start position must become the chart's first-note lead-in.");
    Expect(pumpdx::chart::EventStartBeat(notes.back()) == pumpdx::chart::Beat(11, 4),
        "The final slot must retain its exact sixteenth-note time after the lead-in.");
}

} // namespace

int main() {
    TestFiveLaneSixteenthGridConvertsToTapNotes();

    std::cout << "Legacy .stp importer tests passed.\n";
    return EXIT_SUCCESS;
}
