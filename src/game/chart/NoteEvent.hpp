#pragma once

#include "game/chart/Beat.hpp"
#include "game/chart/PanelLane.hpp"

#include <cstdint>
#include <type_traits>
#include <variant>

namespace pumpdx::chart {

enum class HoldTickMode : std::uint8_t {
    FixedCount,
};

struct HoldTickPolicy final {
    HoldTickMode mode = HoldTickMode::FixedCount;
    std::uint32_t tickCount = 1;

    [[nodiscard]] static constexpr HoldTickPolicy FixedCount(const std::uint32_t count) noexcept {
        return {.mode = HoldTickMode::FixedCount, .tickCount = count};
    }

    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return mode == HoldTickMode::FixedCount && tickCount > 0;
    }
};

struct TapNote final {
    Beat beat{};
    PanelLane lane = PanelLane::Center;
};

struct HoldNote final {
    Beat startBeat{};
    Beat endBeat{};
    PanelLane lane = PanelLane::Center;
    HoldTickPolicy tickPolicy{};
};

using NoteEvent = std::variant<TapNote, HoldNote>;

[[nodiscard]] inline const Beat& EventStartBeat(const NoteEvent& event) {
    return std::visit([](const auto& note) -> const Beat& {
        if constexpr (std::is_same_v<std::decay_t<decltype(note)>, TapNote>) {
            return note.beat;
        } else {
            return note.startBeat;
        }
    }, event);
}

[[nodiscard]] inline PanelLane EventLane(const NoteEvent& event) {
    return std::visit([](const auto& note) {
        return note.lane;
    }, event);
}

} // namespace pumpdx::chart
