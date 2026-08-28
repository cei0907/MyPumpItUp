#include "game/chart/NativeChartWriter.hpp"

#include <fstream>
#include <cmath>
#include <iomanip>
#include <stdexcept>
#include <string_view>

namespace pumpdx::chart {

namespace {

[[nodiscard]] std::string BeatText(const Beat& beat) {
    if (beat.Denominator() == 1) {
        return std::to_string(beat.Numerator());
    }
    return std::to_string(beat.Numerator()) + "/" + std::to_string(beat.Denominator());
}

[[nodiscard]] std::string_view LaneText(const PanelLane lane) {
    switch (lane) {
    case PanelLane::DownLeft: return "SW";
    case PanelLane::UpLeft: return "NW";
    case PanelLane::Center: return "C";
    case PanelLane::UpRight: return "NE";
    case PanelLane::DownRight: return "SE";
    }
    throw std::invalid_argument("Chart contains an invalid panel lane.");
}

} // namespace

std::string NativeChartWriter::Serialize(const Chart& chart) {
    std::ostringstream output;
    output << "schemaVersion=1\n";
    output << "id=" << chart.Id() << '\n';
    output << "delayMilliseconds=" << static_cast<long long>(std::llround(chart.DelaySeconds() * 1000.0)) << "\n\n";
    output << "[tempo]\n";
    output << std::setprecision(15);
    for (const auto& segment : chart.Timing().Segments()) {
        output << BeatText(segment.startBeat) << '=' << segment.beatsPerMinute << '\n';
    }
    output << "\n[notes]\n";
    for (const auto& event : chart.Notes()) {
        std::visit([&output](const auto& note) {
            using Note = std::decay_t<decltype(note)>;
            if constexpr (std::is_same_v<Note, TapNote>) {
                output << "tap=" << LaneText(note.lane) << ',' << BeatText(note.beat) << '\n';
            } else {
                output << "hold=" << LaneText(note.lane) << ',' << BeatText(note.startBeat) << ','
                    << BeatText(note.endBeat) << ',' << note.tickPolicy.tickCount << '\n';
            }
        }, event);
    }
    return output.str();
}

void NativeChartWriter::Save(const Chart& chart, const std::filesystem::path& destinationPath) {
    std::ofstream output(destinationPath, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Unable to write .pdxchart: " + destinationPath.string());
    }
    output << Serialize(chart);
    if (!output) {
        throw std::runtime_error("Unable to finish writing .pdxchart: " + destinationPath.string());
    }
}

} // namespace pumpdx::chart
