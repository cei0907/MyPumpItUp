#pragma once

#include <cstdint>
#include <vector>

namespace pumpdx::video {

struct BgaVideoFrame final {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint64_t serial = 0;
    std::vector<std::uint8_t> bgraPixels;
};

} // namespace pumpdx::video
