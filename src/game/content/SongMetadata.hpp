#pragma once

#include <cstdint>
#include <string>

namespace pumpdx::content {

struct SongMetadata final {
    std::string id;
    std::wstring title;
    std::wstring artist;
    std::wstring difficultyName;
    std::uint8_t difficultyLevel = 0;
};

} // namespace pumpdx::content
