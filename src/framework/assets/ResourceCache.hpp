#pragma once

#include "framework/assets/ThemeManifest.hpp"

#include <filesystem>
#include <string_view>

namespace pumpdx::assets {

class ResourceCache final {
public:
    void LoadTheme(const std::filesystem::path& manifestPath);

    [[nodiscard]] const ThemeManifest& ActiveTheme() const;
    [[nodiscard]] std::filesystem::path Resolve(std::string_view resourceKey) const;

private:
    ThemeManifest activeTheme_{};
    bool hasActiveTheme_ = false;
};

} // namespace pumpdx::assets
