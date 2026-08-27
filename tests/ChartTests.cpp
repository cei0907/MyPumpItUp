#include "game/chart/Chart.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pumpdx::chart::Beat;
using pumpdx::chart::Chart;
using pumpdx::chart::HoldNote;
using pumpdx::chart::HoldTickPolicy;
using pumpdx::chart::NoteEvent;
using pumpdx::chart::PanelLane;
using pumpdx::chart::TapNote;
using pumpdx::chart::TimingMap;

void Expect(const bool condition, const char* message) {
    if (condition) {
        return;
    }

    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

[[nodiscard]] TimingMap MakeTimingMap() {
    return TimingMap({{{0}, 120.0}});
}

void ExpectInvalidChart(const std::vector<NoteEvent>& notes, const char* message) {
    try {
        [[maybe_unused]] Chart chart("invalid", MakeTimingMap(), notes);
    } catch (const std::invalid_argument&) {
        return;
    }

    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

void TestChartNormalizesEventOrder() {
    Chart chart(
        "order-test",
        MakeTimingMap(),
        {
            TapNote{.beat = {4}, .lane = PanelLane::DownRight},
            TapNote{.beat = {1}, .lane = PanelLane::Center},
            TapNote{.beat = {1}, .lane = PanelLane::DownLeft},
        });

    const auto& notes = chart.Notes();
    Expect(notes.size() == 3, "Chart lost note events while normalizing order.");
    Expect(std::get<TapNote>(notes[0]).lane == PanelLane::DownLeft, "Same-beat events must sort by panel lane.");
    Expect(std::get<TapNote>(notes[1]).lane == PanelLane::Center, "Same-beat events must keep canonical panel order.");
    Expect(std::get<TapNote>(notes[2]).beat == Beat(4), "Later note was not sorted after earlier events.");
}

void TestHoldTickPolicySupportsDifferentComboCounts() {
    const HoldNote shortComboHold{
        .startBeat = {4},
        .endBeat = {12},
        .lane = PanelLane::Center,
        .tickPolicy = HoldTickPolicy::FixedCount(10),
    };
    const HoldNote longComboHold{
        .startBeat = {16},
        .endBeat = {24},
        .lane = PanelLane::Center,
        .tickPolicy = HoldTickPolicy::FixedCount(100),
    };

    Expect(shortComboHold.endBeat - shortComboHold.startBeat == longComboHold.endBeat - longComboHold.startBeat,
        "Test holds must have the same visual duration.");
    Expect(shortComboHold.tickPolicy.tickCount == 10, "First hold did not preserve its fixed tick count.");
    Expect(longComboHold.tickPolicy.tickCount == 100, "Second hold did not preserve its fixed tick count.");

    [[maybe_unused]] Chart chart("hold-policy-test", MakeTimingMap(), {shortComboHold, longComboHold});
}

void TestInvalidNotesAreRejected() {
    ExpectInvalidChart({}, "An empty chart must be rejected.");
    ExpectInvalidChart({TapNote{.beat = {-1}, .lane = PanelLane::Center}}, "Notes before beat zero must be rejected.");
    ExpectInvalidChart({HoldNote{
        .startBeat = {4},
        .endBeat = {4},
        .lane = PanelLane::Center,
        .tickPolicy = HoldTickPolicy::FixedCount(1),
    }}, "Zero-length holds must be rejected.");
    ExpectInvalidChart({HoldNote{
        .startBeat = {4},
        .endBeat = {8},
        .lane = PanelLane::Center,
        .tickPolicy = HoldTickPolicy::FixedCount(0),
    }}, "Holds with zero ticks must be rejected.");
    ExpectInvalidChart({
        HoldNote{
            .startBeat = {4},
            .endBeat = {12},
            .lane = PanelLane::Center,
            .tickPolicy = HoldTickPolicy::FixedCount(10),
        },
        HoldNote{
            .startBeat = {8},
            .endBeat = {16},
            .lane = PanelLane::Center,
            .tickPolicy = HoldTickPolicy::FixedCount(10),
        },
    }, "Overlapping holds on the same lane must be rejected.");
    ExpectInvalidChart({
        HoldNote{
            .startBeat = {4},
            .endBeat = {12},
            .lane = PanelLane::Center,
            .tickPolicy = HoldTickPolicy::FixedCount(10),
        },
        TapNote{.beat = {8}, .lane = PanelLane::Center},
    }, "Tap notes inside a hold on the same lane must be rejected.");
}

} // namespace

int main() {
    TestChartNormalizesEventOrder();
    TestHoldTickPolicySupportsDifferentComboCounts();
    TestInvalidNotesAreRejected();

    std::cout << "Chart model tests passed.\n";
    return EXIT_SUCCESS;
}
