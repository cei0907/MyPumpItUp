#pragma once

namespace pumpdx::render::layout {

// Logical 1280x720 single-player gameplay layout. Rendering and chart projection share these values.
// The narrow, top-anchored receptor band leaves the BGA visible around the rising note field.
inline constexpr float kFieldLeft = 162.0F;
inline constexpr float kLaneWidth = 108.0F;
inline constexpr float kLaneGap = 8.0F;
inline constexpr float kFieldTop = 78.0F;
inline constexpr float kFieldBottom = 672.0F;
inline constexpr float kReceptorY = 152.0F;
inline constexpr float kFieldRight = kFieldLeft + 5.0F * kLaneWidth + 4.0F * kLaneGap;

inline constexpr float kGaugeLeft = 188.0F;
inline constexpr float kGaugeRight = 680.0F;
inline constexpr float kGaugeTop = 20.0F;
inline constexpr float kGaugeBottom = 46.0F;

} // namespace pumpdx::render::layout
