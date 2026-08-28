#include "game/chart/LegacyStpConverter.hpp"
#include "game/chart/NativeChartWriter.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

int main(const int argumentCount, char* arguments[]) {
    if (argumentCount != 5 && argumentCount != 6) {
        std::cerr << "Usage: PumpDXChartConvert <source.stp> <output.pdxchart> <chart-id> <legacy-start-position> [hold-overlay.pdxchart]\n";
        return 1;
    }

    try {
        const std::filesystem::path sourcePath(arguments[1]);
        const std::filesystem::path outputPath(arguments[2]);
        const auto startPosition = std::stoi(arguments[4]);
        const auto overlayPath = argumentCount == 6
            ? std::optional<std::filesystem::path>(std::filesystem::path(arguments[5]))
            : std::nullopt;
        const auto chart = pumpdx::chart::LegacyStpConverter::Convert(
            sourcePath, arguments[3], startPosition, overlayPath);
        std::filesystem::create_directories(outputPath.parent_path());
        pumpdx::chart::NativeChartWriter::Save(chart, outputPath);
        std::cout << "Converted " << sourcePath.string() << " to " << outputPath.string()
            << " (" << chart.Notes().size() << " events).\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Conversion failed: " << exception.what() << '\n';
        return 1;
    }
}
