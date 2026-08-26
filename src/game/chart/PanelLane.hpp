#pragma once

#include <cstdint>

namespace pumpdx::chart {

enum class PanelLane : std::uint8_t {
    DownLeft,
    UpLeft,
    Center,
    UpRight,
    DownRight,
};

[[nodiscard]] constexpr bool IsValidPanelLane(const PanelLane lane) noexcept {
    switch (lane) {
    case PanelLane::DownLeft:
    case PanelLane::UpLeft:
    case PanelLane::Center:
    case PanelLane::UpRight:
    case PanelLane::DownRight:
        return true;
    }

    return false;
}

} // namespace pumpdx::chart
