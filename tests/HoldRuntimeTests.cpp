#include "game/gameplay/GameplayRuntime.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>

namespace {

class MutableSongClock final : public pumpdx::gameplay::SongClock {
public:
    [[nodiscard]] double Seconds() const noexcept override {
        return seconds_;
    }

    void SetSeconds(const double seconds) noexcept {
        seconds_ = seconds;
    }

private:
    double seconds_ = 0.0;
};

void Expect(const bool condition, const char* message) {
    if (condition) {
        return;
    }
    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

[[nodiscard]] pumpdx::chart::Chart MakeHoldChart(const std::uint32_t tickCount = 4) {
    using namespace pumpdx::chart;
    return Chart(
        "hold-runtime-test",
        TimingMap({{{0}, 60.0}}),
        {HoldNote{
            .startBeat = {1},
            .endBeat = {5},
            .lane = PanelLane::Center,
            .tickPolicy = HoldTickPolicy::FixedCount(tickCount),
        }});
}

void TestCompletedHoldAwardsHeadAndEverySustainPoint() {
    auto clock = std::make_unique<MutableSongClock>();
    auto* clockView = clock.get();
    pumpdx::gameplay::GameplayRuntime runtime(MakeHoldChart(), std::move(clock));

    clockView->SetSeconds(1.0);
    const auto inactiveItems = runtime.BuildRenderItemsForCurrentTime();
    Expect(inactiveItems.size() == 1 && !inactiveItems.front().isHoldActive,
        "An unpressed hold body must remain visually inactive.");
    runtime.SetPanelPressed(pumpdx::chart::PanelLane::Center, true);
    const auto activeItems = runtime.BuildRenderItemsForCurrentTime();
    Expect(activeItems.size() == 1 && activeItems.front().isHoldActive,
        "A held body must expose its active visual state.");
    for (const auto seconds : {2.0, 3.0, 4.0, 5.0}) {
        clockView->SetSeconds(seconds);
        runtime.Update();
    }

    Expect(runtime.Score().Score() == 5000, "Hold head and all four sustain points must score PERFECT.");
    Expect(runtime.Score().CurrentCombo() == 5, "A completed hold must keep one continuous combo.");
    Expect(runtime.Score().MaxCombo() == 5, "Completed hold combo length was not preserved.");
    Expect(runtime.Score().HoldTicks() == 4, "Every configured sustain point must be counted as a hold tick.");
    Expect(runtime.BuildResultSummary().judgedNotes == 5, "Hold head plus sustain points must be included in the result.");
}

void TestReleaseMissesOnlyTheGapAndRepressResumes() {
    auto clock = std::make_unique<MutableSongClock>();
    auto* clockView = clock.get();
    pumpdx::gameplay::GameplayRuntime runtime(MakeHoldChart(), std::move(clock));

    clockView->SetSeconds(1.0);
    runtime.SetPanelPressed(pumpdx::chart::PanelLane::Center, true);
    clockView->SetSeconds(2.0);
    runtime.Update();
    runtime.SetPanelPressed(pumpdx::chart::PanelLane::Center, false);
    clockView->SetSeconds(3.0);
    runtime.Update();
    clockView->SetSeconds(3.2);
    runtime.SetPanelPressed(pumpdx::chart::PanelLane::Center, true);
    clockView->SetSeconds(5.0);
    runtime.Update();

    Expect(runtime.Score().Score() == 4000, "Re-pressing must score the remaining hold points after the missed gap.");
    Expect(runtime.Score().CurrentCombo() == 2, "Re-pressed hold points must form a new combo after the missed gap.");
    Expect(runtime.Score().MaxCombo() == 2, "The successful parts on either side of the gap must preserve their max combo.");
    Expect(runtime.Score().HoldTicks() == 4, "Every scheduled sustain point, including the missed gap, must be counted.");
    Expect(runtime.BuildResultSummary().judgedNotes == 5, "Release and re-press must still resolve every scheduled sustain point.");
}

void TestLateBodyPressStartsFromTheNextAvailableTick() {
    auto clock = std::make_unique<MutableSongClock>();
    auto* clockView = clock.get();
    pumpdx::gameplay::GameplayRuntime runtime(MakeHoldChart(), std::move(clock));

    clockView->SetSeconds(2.2);
    runtime.SetPanelPressed(pumpdx::chart::PanelLane::Center, true);
    runtime.Update();
    clockView->SetSeconds(5.0);
    runtime.Update();

    Expect(runtime.Score().Score() == 3000, "Late body press must score only the remaining three sustain points.");
    Expect(runtime.Score().CurrentCombo() == 3, "Late body press must start a combo at the next available tick.");
    Expect(runtime.Score().HoldTicks() == 3, "Ticks before the late body press must not be retroactively judged.");
    Expect(runtime.BuildResultSummary().judgedNotes == 4, "Late catch must include the missed head and remaining sustain points.");
}

} // namespace

int main() {
    TestCompletedHoldAwardsHeadAndEverySustainPoint();
    TestReleaseMissesOnlyTheGapAndRepressResumes();
    TestLateBodyPressStartsFromTheNextAvailableTick();
    std::cout << "Hold runtime tests passed.\n";
    return EXIT_SUCCESS;
}
