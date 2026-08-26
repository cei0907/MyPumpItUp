#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pumpdx::assets {

struct ThemePalette final {
    std::array<float, 4> panel{};
    std::array<float, 4> accent{};
    std::array<float, 4> heading{};
    std::array<float, 4> detail{};
    std::array<float, 4> instruction{};
};

class ThemeManifest final {
public:
    [[nodiscard]] static ThemeManifest LoadFromFile(const std::filesystem::path& manifestPath);

    [[nodiscard]] const std::string& Id() const noexcept;
    [[nodiscard]] const ThemePalette& Palette() const noexcept;
    [[nodiscard]] std::filesystem::path ResolveResource(std::string_view key) const;

private:
    std::string id_;
    ThemePalette palette_{};
    std::filesystem::path rootDirectory_;
    std::unordered_map<std::string, std::filesystem::path> resources_;
};

} // namespace pumpdx::assets
