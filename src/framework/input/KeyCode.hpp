#pragma once

#include <cstdint>
#include <optional>

namespace pumpdx::input {

inline constexpr std::uint32_t kConfirm = 0x0D;
inline constexpr std::uint32_t kCancel = 0x1B;

enum class FivePanelInput : std::uint8_t {
    DownLeft,
    UpLeft,
    Center,
    UpRight,
    DownRight,
};

[[nodiscard]] constexpr std::optional<FivePanelInput> ToFivePanelInput(const std::uint32_t virtualKey) noexcept {
    switch (virtualKey) {
    case 'Z': return FivePanelInput::DownLeft;
    case 'Q': return FivePanelInput::UpLeft;
    case 'S': return FivePanelInput::Center;
    case 'E': return FivePanelInput::UpRight;
    case 'C': return FivePanelInput::DownRight;
    default: return std::nullopt;
    }
}

} // namespace pumpdx::input
