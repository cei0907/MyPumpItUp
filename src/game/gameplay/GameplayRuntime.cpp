#include "game/gameplay/GameplayRuntime.hpp"

#include "game/gameplay/DebugSongClock.hpp"
#include "framework/render/GameplayLayout.hpp"

#include <algorithm>
#include <utility>

namespace pumpdx::gameplay {

namespace {

constexpr float kScrollPixelsPerSecond = 230.0F;
constexpr float kVisibleTop = 88.0F;
constexpr float kVisibleBottom = 685.0F;

[[nodiscard]] float ToScreenY(const double eventSeconds, const double songTimeSeconds) {
    return render::layout::kReceptorY + static_cast<float>((eventSeconds - songTimeSeconds) * kScrollPixelsPerSecond);
}

} // namespace

GameplayRuntime::GameplayRuntime(const chart::Chart& chart, std::unique_ptr<SongClock> songClock)
    : songClock_(songClock ? std::move(songClock) : std::make_unique<DebugSongClock>())
    , timeline_(CompileTimeline(chart))
    , judgementEngine_([this] {
        std::vector<JudgableNote> notes;
        notes.reserve(timeline_.size());
        for (const auto& note : timeline_) {
            notes.push_back({.lane = note.lane, .timeSeconds = note.startSeconds});
        }
        return notes;
    }()) {
}

void GameplayRuntime::SetPanelPressed(const chart::PanelLane lane, const bool pressed) noexcept {
    const auto index = static_cast<std::size_t>(lane);
    const auto wasPressed = pressedPanels_[index];
    pressedPanels_[index] = pressed;
    const auto songTimeSeconds = SongTimeSeconds();
    if (pressed && !wasPressed) {
        if (const auto event = judgementEngine_.TryJudge(lane, songTimeSeconds); event.has_value()) {
            ResolveHeadJudgement(*event);
        }
        TryActivateHoldFromBody(lane, songTimeSeconds);
        UpdateHoldVisualStart(lane, songTimeSeconds);
    } else if (!pressed && wasPressed) {
        UpdateHoldVisualStart(lane, songTimeSeconds);
    }
}

void GameplayRuntime::Update() {
    const auto songTimeSeconds = SongTimeSeconds();
    for (const auto& miss : judgementEngine_.CollectMisses(songTimeSeconds)) {
        ResolveHeadJudgement(miss);
    }
    ProcessHoldTicks(songTimeSeconds);
}

double GameplayRuntime::SongTimeSeconds() const noexcept {
    return songClock_->Seconds();
}

const std::array<bool, 5>& GameplayRuntime::PressedPanels() const noexcept {
    return pressedPanels_;
}

const ScoreState& GameplayRuntime::Score() const noexcept {
    return scoreState_;
}

const EnergyGauge& GameplayRuntime::Energy() const noexcept {
    return energyGauge_;
}

session::GameplaySummary GameplayRuntime::BuildResultSummary() const noexcept {
    auto summary = scoreState_.BuildSummary();
    summary.cleared = energyGauge_.IsAlive();
    return summary;
}

std::vector<render::GameplayRenderItem> GameplayRuntime::BuildRenderItems(const double songTimeSeconds) const {
    std::vector<render::GameplayRenderItem> items;
    items.reserve(timeline_.size());

    for (std::size_t index = 0; index < timeline_.size(); ++index) {
        const auto& note = timeline_[index];
        if (!note.isHold && judgementEngine_.IsResolved(index)) {
            continue;
        }
        if (note.isHold && note.holdFinishedSuccessfully) {
            continue;
        }
        const auto headY = ToScreenY(note.startSeconds, songTimeSeconds);
        const auto tailY = note.isHold ? ToScreenY(note.endSeconds, songTimeSeconds) : headY;
        const auto hasConsumedHead = note.isHold
            && note.holdActivated
            && headY < render::layout::kReceptorY;
        const auto hasRemainingHoldTick = note.isHold
            && note.nextHoldTick < note.holdTickSeconds.size();
        const auto isHoldBeingHeld = note.isHold
            && note.holdActivated
            && hasRemainingHoldTick
            && songTimeSeconds >= note.startSeconds
            && songTimeSeconds <= note.endSeconds
            && pressedPanels_[static_cast<std::size_t>(note.lane)];
        const auto holdBodyStartY = hasConsumedHead
            ? (isHoldBeingHeld
                ? render::layout::kReceptorY
                : ToScreenY(note.visualBodyStartSeconds, songTimeSeconds))
            : headY;
        const auto top = std::min(holdBodyStartY, tailY);
        const auto bottom = std::max(holdBodyStartY, tailY);
        if (bottom < kVisibleTop || top > kVisibleBottom) {
            continue;
        }

        items.push_back({
            .lane = static_cast<std::uint8_t>(note.lane),
            .headY = headY,
            .holdBodyStartY = holdBodyStartY,
            .tailY = tailY,
            .isHold = note.isHold,
            .isHoldActive = note.isHold
                && note.holdActivated
                && hasRemainingHoldTick
                && isHoldBeingHeld,
            .isHoldDamaged = note.isHold && note.holdHasMissedTick,
            .showHead = !hasConsumedHead,
        });
    }

    return items;
}

void GameplayRuntime::ResolveHeadJudgement(JudgementEvent event) noexcept {
    if (event.noteIndex < timeline_.size() && timeline_[event.noteIndex].isHold) {
        event.source = JudgementSource::HoldHead;
        auto& hold = timeline_[event.noteIndex];
        hold.holdActivated = hold.holdActivated || event.judgement != Judgement::Miss;
    }
    Apply(event);
}

void GameplayRuntime::ProcessHoldTicks(const double songTimeSeconds) {
    for (std::size_t noteIndex = 0; noteIndex < timeline_.size(); ++noteIndex) {
        auto& hold = timeline_[noteIndex];
        if (!hold.isHold || !hold.holdActivated) {
            continue;
        }

        while (hold.nextHoldTick < hold.holdTickSeconds.size()
            && hold.holdTickSeconds[hold.nextHoldTick] <= songTimeSeconds) {
            const bool isEnd = hold.nextHoldTick + 1 == hold.holdTickSeconds.size();
            const bool isHeld = pressedPanels_[static_cast<std::size_t>(hold.lane)];
            Apply({
                .noteIndex = noteIndex,
                .lane = hold.lane,
                .judgement = isHeld ? Judgement::Perfect : Judgement::Miss,
                .timingErrorSeconds = 0.0,
                .source = isEnd ? JudgementSource::HoldEnd : JudgementSource::HoldTick,
            });
            if (!isHeld) {
                hold.holdHasMissedTick = true;
            } else if (isEnd) {
                hold.holdFinishedSuccessfully = true;
            }
            ++hold.nextHoldTick;
        }
    }
}

void GameplayRuntime::TryActivateHoldFromBody(const chart::PanelLane lane, const double songTimeSeconds) noexcept {
    for (auto& hold : timeline_) {
        if (!hold.isHold || hold.lane != lane || hold.holdActivated
            || songTimeSeconds < hold.startSeconds || songTimeSeconds > hold.endSeconds) {
            continue;
        }

        hold.holdActivated = true;
        while (hold.nextHoldTick < hold.holdTickSeconds.size()
            && hold.holdTickSeconds[hold.nextHoldTick] < songTimeSeconds) {
            ++hold.nextHoldTick;
        }
        return;
    }
}

void GameplayRuntime::UpdateHoldVisualStart(const chart::PanelLane lane, const double songTimeSeconds) noexcept {
    for (auto& hold : timeline_) {
        if (hold.isHold && hold.lane == lane && hold.holdActivated
            && songTimeSeconds >= hold.startSeconds && songTimeSeconds <= hold.endSeconds) {
            hold.visualBodyStartSeconds = songTimeSeconds;
        }
    }
}

void GameplayRuntime::Apply(const JudgementEvent& event) noexcept {
    scoreState_.Apply(event);
    energyGauge_.Apply(event);
}

std::vector<render::GameplayRenderItem> GameplayRuntime::BuildRenderItemsForCurrentTime() const {
    return BuildRenderItems(SongTimeSeconds());
}

std::vector<GameplayRuntime::TimelineNote> GameplayRuntime::CompileTimeline(const chart::Chart& chart) {
    std::vector<TimelineNote> timeline;
    timeline.reserve(chart.Notes().size());

    for (const auto& event : chart.Notes()) {
        std::visit([&timeline, &chart](const auto& note) {
            using Note = std::decay_t<decltype(note)>;
            if constexpr (std::is_same_v<Note, chart::TapNote>) {
                const auto seconds = chart.Timing().SecondsAt(note.beat);
                timeline.push_back({
                    .lane = note.lane,
                    .startSeconds = seconds,
                    .endSeconds = seconds,
                    .visualBodyStartSeconds = seconds,
                    .isHold = false,
                });
            } else {
                std::vector<double> holdTickSeconds;
                holdTickSeconds.reserve(note.tickPolicy.tickCount);
                const auto startSeconds = chart.Timing().SecondsAt(note.startBeat);
                const auto endSeconds = chart.Timing().SecondsAt(note.endBeat);
                const auto durationSeconds = endSeconds - startSeconds;
                for (std::uint32_t tickIndex = 1; tickIndex <= note.tickPolicy.tickCount; ++tickIndex) {
                    holdTickSeconds.push_back(startSeconds + durationSeconds
                        * static_cast<double>(tickIndex) / static_cast<double>(note.tickPolicy.tickCount));
                }
                timeline.push_back({
                    .lane = note.lane,
                    .startSeconds = startSeconds,
                    .endSeconds = endSeconds,
                    .visualBodyStartSeconds = startSeconds,
                    .isHold = true,
                    .holdTickSeconds = std::move(holdTickSeconds),
                });
            }
        }, event);
    }

    return timeline;
}

} // namespace pumpdx::gameplay
