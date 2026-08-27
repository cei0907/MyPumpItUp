#include "game/chart/NativeChartLoader.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Expect(const bool condition, const char* message) {
    if (condition) {
        return;
    }
    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

void TestNativeChartLoadsTupletsAndVariableHoldTicks() {
    const auto chart = pumpdx::chart::NativeChartLoader::Load(PUMP_DX_TEST_NATIVE_CHART);
    const auto& notes = chart.Notes();

    Expect(chart.Id() == "native-hold-test", "Native chart id was not loaded.");
    Expect(notes.size() == 4, "Native chart lost note events.");
    Expect(pumpdx::chart::EventStartBeat(notes[1]) == pumpdx::chart::Beat(1, 3), "Triplet beat was not loaded exactly.");

    const auto* firstHold = std::get_if<pumpdx::chart::HoldNote>(&notes[2]);
    const auto* secondHold = std::get_if<pumpdx::chart::HoldNote>(&notes[3]);
    Expect(firstHold != nullptr && firstHold->tickPolicy.tickCount == 10, "First hold tick count was not preserved.");
    Expect(secondHold != nullptr && secondHold->startBeat == pumpdx::chart::Beat(52, 3), "Mixed-fraction hold beat was not loaded exactly.");
    Expect(secondHold != nullptr && secondHold->tickPolicy.tickCount == 100, "Second hold tick count was not preserved.");
    Expect(chart.Timing().SecondsAt(pumpdx::chart::Beat(17)) > 8.0, "Second tempo segment was not loaded.");
}

void TestMalformedNativeChartReportsAnError() {
    try {
        [[maybe_unused]] const auto chart = pumpdx::chart::NativeChartLoader::Load("does-not-exist.pdxchart");
    } catch (const std::runtime_error&) {
        return;
    }
    std::cerr << "Missing native chart must report an error.\n";
    std::exit(EXIT_FAILURE);
}

} // namespace

int main() {
    TestNativeChartLoadsTupletsAndVariableHoldTicks();
    TestMalformedNativeChartReportsAnError();
    std::cout << "Native chart loader tests passed.\n";
    return EXIT_SUCCESS;
}
