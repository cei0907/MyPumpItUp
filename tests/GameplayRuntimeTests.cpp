#include "game/gameplay/GameplayRuntime.hpp"
#include "framework/render/GameplayLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

namespace {

constexpr float kTolerance = 0.001F;

class FixedSongClock final : public pumpdx::gameplay::SongClock {
public:
    explicit FixedSongClock(const double seconds)
        : seconds_(seconds) {
    }

    [[nodiscard]] double Seconds() const noexcept override {
        return seconds_;
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

[[nodiscard]] pumpdx::chart::Chart MakeChart() {
    using namespace pumpdx::chart;

    return Chart(
        "runtime-test",
        TimingMap({{{0}, 120.0}}),
        {
            TapNote{.beat = {0}, .lane = PanelLane::DownLeft},
            HoldNote{
                .startBeat = {4},
                .endBeat = {12},
                .lane = PanelLane::Center,
                .tickPolicy = HoldTickPolicy::FixedCount(10),
            },
        });
}

void TestChartTimeProjectsToLogicalField() {
    const auto chart = MakeChart();
    const pumpdx::gameplay::GameplayRuntime runtime(chart);

    const auto startItems = runtime.BuildRenderItems(0.0);
    Expect(startItems.size() == 2, "Top-receptor layout should preview the upcoming hold at start time.");
    const auto tap = std::find_if(startItems.begin(), startItems.end(), [](const auto& item) {
        return !item.isHold;
    });
    Expect(tap != startItems.end() && tap->lane == 0, "Tap note projected to the wrong panel lane.");
    Expect(std::abs(tap->headY - pumpdx::render::layout::kReceptorY) < kTolerance,
        "Tap note did not project onto the receptor at its chart time.");

    const auto holdItems = runtime.BuildRenderItems(3.0);
    Expect(holdItems.size() == 1, "The hold note should be visible while its body crosses the field.");
    Expect(holdItems.front().isHold, "Hold note lost its visual hold flag.");
    Expect(holdItems.front().tailY > holdItems.front().headY, "Hold body must extend from start beat to end beat.");
}

void TestPanelStateIsIndependentOfRenderingTime() {
    const auto chart = MakeChart();
    pumpdx::gameplay::GameplayRuntime runtime(chart);

    runtime.SetPanelPressed(pumpdx::chart::PanelLane::UpRight, true);
    Expect(runtime.PressedPanels()[3], "Pressed panel state was not recorded.");
    runtime.SetPanelPressed(pumpdx::chart::PanelLane::UpRight, false);
    Expect(!runtime.PressedPanels()[3], "Released panel state was not cleared.");
}

void TestInputEdgeUsesSongTimeForJudgement() {
    const auto chart = MakeChart();
    pumpdx::gameplay::GameplayRuntime runtime(chart, std::make_unique<FixedSongClock>(0.0));

    runtime.SetPanelPressed(pumpdx::chart::PanelLane::DownLeft, true);
    Expect(runtime.Score().Score() == 1000, "Input at the note's song time must earn a PERFECT score.");
    Expect(runtime.Score().CurrentCombo() == 1, "A successful input edge must start the combo.");

    runtime.SetPanelPressed(pumpdx::chart::PanelLane::DownLeft, true);
    Expect(runtime.Score().Score() == 1000, "Repeated key-down while held must not judge twice.");
}

} // namespace

int main() {
    TestChartTimeProjectsToLogicalField();
    TestPanelStateIsIndependentOfRenderingTime();
    TestInputEdgeUsesSongTimeForJudgement();

    std::cout << "Gameplay runtime tests passed.\n";
    return EXIT_SUCCESS;
}
