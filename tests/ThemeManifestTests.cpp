#include "framework/assets/ResourceCache.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

void Expect(const bool condition, const char* message) {
    if (condition) {
        return;
    }

    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

void WriteTextFile(const std::filesystem::path& path, const std::string_view contents) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Unable to create theme test fixture.");
    }
    output << contents;
}

void TestThemeLoadAndResourceResolution() {
    const auto fixtureDirectory = std::filesystem::temp_directory_path() / "pumpdx_theme_manifest_tests";
    std::filesystem::remove_all(fixtureDirectory);
    std::filesystem::create_directories(fixtureDirectory / "ui");

    WriteTextFile(
        fixtureDirectory / "theme.json",
        R"({
            "schemaVersion": "1",
            "id": "test-theme",
            "palette.panel": "#11223344",
            "palette.accent": "#556677",
            "palette.heading": "#FFFFFFFF",
            "palette.detail": "#ABCDEF",
            "palette.instruction": "#010203",
            "resources.overlay.accent": "ui/accent.txt"
        })");
    WriteTextFile(fixtureDirectory / "ui" / "accent.txt", "fixture");

    pumpdx::assets::ResourceCache cache;
    cache.LoadTheme(fixtureDirectory / "theme.json");

    Expect(cache.ActiveTheme().Id() == "test-theme", "Theme identifier was not loaded.");
    Expect(std::abs(cache.ActiveTheme().Palette().panel[0] - (17.0F / 255.0F)) < 0.0001F, "Theme palette was not parsed.");
    Expect(cache.Resolve("overlay.accent") == fixtureDirectory / "ui" / "accent.txt", "Theme resource did not resolve.");

    std::filesystem::remove_all(fixtureDirectory);
}

void TestInvalidResourcePathIsRejected() {
    const auto fixtureDirectory = std::filesystem::temp_directory_path() / "pumpdx_theme_manifest_invalid_tests";
    std::filesystem::remove_all(fixtureDirectory);
    std::filesystem::create_directories(fixtureDirectory);

    WriteTextFile(
        fixtureDirectory / "theme.json",
        R"({
            "schemaVersion": "1",
            "id": "invalid-theme",
            "palette.panel": "#000000",
            "palette.accent": "#000000",
            "palette.heading": "#000000",
            "palette.detail": "#000000",
            "palette.instruction": "#000000",
            "resources.invalid": "../outside.txt"
        })");

    try {
        pumpdx::assets::ResourceCache cache;
        cache.LoadTheme(fixtureDirectory / "theme.json");
    } catch (const std::runtime_error&) {
        std::filesystem::remove_all(fixtureDirectory);
        return;
    }

    std::filesystem::remove_all(fixtureDirectory);
    Expect(false, "Theme resource paths must not leave the theme directory.");
}

} // namespace

int main() {
    TestThemeLoadAndResourceResolution();
    TestInvalidResourcePathIsRejected();

    std::cout << "Theme manifest tests passed.\n";
    return EXIT_SUCCESS;
}
