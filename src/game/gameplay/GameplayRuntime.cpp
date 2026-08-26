#include "game/gameplay/GameplayRuntime.hpp"

#include "game/gameplay/DebugSongClock.hpp"

#include <algorithm>
#include <utility>

namespace pumpdx::gameplay {

namespace {

constexpr float kReceptorY = 525.0F;
constexpr float kScrollPixelsPerSecond = 230.0F;
constexpr float kVisibleTop = 88.0F;
constexpr float kVisibleBottom = 685.0F;

[[nodiscard]] float ToScreenY(const double eventSeconds, const double songTimeSeconds) {
    return kReceptorY + static_cast<float>((eventSeconds - songTimeSeconds) * kScrollPixelsPerSecond);
}

} // namespace

GameplayRuntime::GameplayRuntime(const chart::Chart& chart)
    : songClock_(std::make_unique<DebugSongClock>())
    , timeline_(CompileTimeline(chart)) {
}

void GameplayRuntime::SetPanelPressed(const chart::PanelLane lane, const bool pressed) noexcept {
    pressedPanels_[static_cast<std::size_t>(lane)] = pressed;
}

double GameplayRuntime::SongTimeSeconds() const noexcept {
    return songClock_->Seconds();
}

const std::array<bool, 5>& GameplayRuntime::PressedPanels() const noexcept {
    return pressedPanels_;
}

std::vector<render::GameplayRenderItem> GameplayRuntime::BuildRenderItems(const double songTimeSeconds) const {
    std::vector<render::GameplayRenderItem> items;
    items.reserve(timeline_.size());

    for (const auto& note : timeline_) {
        const auto headY = ToScreenY(note.startSeconds, songTimeSeconds);
        const auto tailY = note.isHold ? ToScreenY(note.endSeconds, songTimeSeconds) : headY;
        const auto top = std::min(headY, tailY);
        const auto bottom = std::max(headY, tailY);
        if (bottom < kVisibleTop || top > kVisibleBottom) {
            continue;
        }

        items.push_back({
            .lane = static_cast<std::uint8_t>(note.lane),
            .headY = headY,
            .tailY = tailY,
            .isHold = note.isHold,
        });
    }

    return items;
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
                    .isHold = false,
                });
            } else {
                timeline.push_back({
                    .lane = note.lane,
                    .startSeconds = chart.Timing().SecondsAt(note.startBeat),
                    .endSeconds = chart.Timing().SecondsAt(note.endBeat),
                    .isHold = true,
                });
            }
        }, event);
    }

    return timeline;
}

} // namespace pumpdx::gameplay
