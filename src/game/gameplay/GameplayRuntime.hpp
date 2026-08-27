#pragma once

#include "framework/render/SceneOverlayRenderer.hpp"
#include "game/chart/Chart.hpp"
#include "game/gameplay/Judgement.hpp"
#include "game/gameplay/EnergyGauge.hpp"
#include "game/gameplay/GameplayEffectPool.hpp"
#include "game/gameplay/ScoreState.hpp"
#include "game/gameplay/SongClock.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace pumpdx::gameplay {

class GameplayRuntime final {
public:
    explicit GameplayRuntime(const chart::Chart& chart, std::unique_ptr<SongClock> songClock = nullptr);

    void SetPanelPressed(chart::PanelLane lane, bool pressed) noexcept;
    void Update();

    [[nodiscard]] double SongTimeSeconds() const noexcept;
    [[nodiscard]] const std::array<bool, 5>& PressedPanels() const noexcept;
    [[nodiscard]] const ScoreState& Score() const noexcept;
    [[nodiscard]] const EnergyGauge& Energy() const noexcept;
    [[nodiscard]] session::GameplaySummary BuildResultSummary() const noexcept;
    [[nodiscard]] std::vector<render::GameplayRenderItem> BuildRenderItems(double songTimeSeconds) const;
    [[nodiscard]] std::vector<render::GameplayRenderItem> BuildRenderItemsForCurrentTime() const;
    [[nodiscard]] render::GameplayFeedback BuildFeedbackForCurrentTime() const noexcept;

private:
    struct TimelineNote final {
        chart::PanelLane lane = chart::PanelLane::Center;
        double startSeconds = 0.0;
        double endSeconds = 0.0;
        double visualBodyStartSeconds = 0.0;
        bool isHold = false;
        std::vector<double> holdTickSeconds;
        std::size_t nextHoldTick = 0;
        bool holdActivated = false;
        bool holdHasMissedTick = false;
        bool holdFinishedSuccessfully = false;
    };

    [[nodiscard]] static std::vector<TimelineNote> CompileTimeline(const chart::Chart& chart);
    void ResolveHeadJudgement(JudgementEvent event) noexcept;
    void ProcessHoldTicks(double songTimeSeconds);
    void TryActivateHoldFromBody(chart::PanelLane lane, double songTimeSeconds) noexcept;
    void UpdateHoldVisualStart(chart::PanelLane lane, double songTimeSeconds) noexcept;
    void Apply(const JudgementEvent& event) noexcept;

    std::unique_ptr<SongClock> songClock_;
    std::vector<TimelineNote> timeline_;
    JudgementEngine judgementEngine_;
    ScoreState scoreState_;
    EnergyGauge energyGauge_;
    GameplayEffectPool effectPool_;
    std::array<bool, 5> pressedPanels_{};
};

} // namespace pumpdx::gameplay
