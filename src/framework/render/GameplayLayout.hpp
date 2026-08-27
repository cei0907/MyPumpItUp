#pragma once

namespace pumpdx::render::layout {

// Logical 1280x720 single-player gameplay layout. Rendering and chart projection share these values.
inline constexpr float kFieldLeft = 290.0F;
inline constexpr float kLaneWidth = 132.0F;
inline constexpr float kLaneGap = 10.0F;
inline constexpr float kFieldTop = 156.0F;
inline constexpr float kFieldBottom = 672.0F;
inline constexpr float kReceptorY = 498.0F;
inline constexpr float kFieldRight = kFieldLeft + 5.0F * kLaneWidth + 4.0F * kLaneGap;

inline constexpr float kGaugeLeft = 332.0F;
inline constexpr float kGaugeRight = 948.0F;
inline constexpr float kGaugeTop = 106.0F;
inline constexpr float kGaugeBottom = 134.0F;

} // namespace pumpdx::render::layout
