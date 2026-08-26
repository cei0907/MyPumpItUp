#include "framework/assets/ResourceCache.hpp"

#include <stdexcept>

namespace pumpdx::assets {

void ResourceCache::LoadTheme(const std::filesystem::path& manifestPath) {
    activeTheme_ = ThemeManifest::LoadFromFile(manifestPath);
    hasActiveTheme_ = true;
}

const ThemeManifest& ResourceCache::ActiveTheme() const {
    if (!hasActiveTheme_) {
        throw std::logic_error("No active theme has been loaded.");
    }
    return activeTheme_;
}

std::filesystem::path ResourceCache::Resolve(const std::string_view resourceKey) const {
    return ActiveTheme().ResolveResource(resourceKey);
}

} // namespace pumpdx::assets
