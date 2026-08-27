#include "game/chart/LegacyStpImporter.hpp"

#include <array>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pumpdx::chart {

Chart LegacyStpImporter::LoadTapChart(
    const std::filesystem::path& sourcePath,
    std::string chartId,
    const int legacyStartPosition) {
    std::ifstream input(sourcePath);
    if (!input) {
        throw std::runtime_error("Unable to open legacy .stp chart: " + sourcePath.string());
    }

    double beatsPerMinute = 0.0;
    std::size_t slotCount = 0;
    if (!(input >> beatsPerMinute >> slotCount) || beatsPerMinute <= 0.0 || slotCount == 0) {
        throw std::runtime_error("Legacy .stp chart has an invalid BPM or slot count.");
    }
    if (legacyStartPosition < 83) {
        throw std::runtime_error("Legacy .stp start position cannot be below the receptor position.");
    }
    input.ignore(8192, '\n');

    std::array<std::string, 5> lanes;
    for (std::size_t laneIndex = 0; laneIndex < lanes.size(); ++laneIndex) {
        auto& lane = lanes[laneIndex];
        const auto read = static_cast<bool>(std::getline(input, lane));
        if (!read || lane.empty()) {
            throw std::runtime_error("Legacy .stp chart must contain five equal-length lane strings.");
        }
    }

    const auto actualSlotCount = lanes.front().size();
    for (const auto& lane : lanes) {
        if (lane.size() < actualSlotCount) {
            throw std::runtime_error("Legacy .stp chart lanes cannot be shorter than the first lane.");
        }
    }
    // The legacy loader overwrote the header with `firstLane.length() + 1` after reading,
    // then used only the first lane's length even if another lane had a trailing symbol.
    // Accept both the historical header and a corrected exact-length header.
    if (slotCount != actualSlotCount && slotCount != actualSlotCount + 1) {
        throw std::runtime_error("Legacy .stp header slot count does not match its lane strings.");
    }

    std::vector<NoteEvent> notes;
    // The old renderer used y = startPosition + slot * 45 and velocity = 3 * BPM.
    // Its visual lead-in therefore becomes (startPosition - 83) / 180 musical beats.
    const Beat leadInBeat(legacyStartPosition - 83, 180);
    for (std::size_t slot = 0; slot < actualSlotCount; ++slot) {
        for (std::size_t lane = 0; lane < lanes.size(); ++lane) {
            const auto symbol = lanes[lane][slot];
            if (symbol == '0') {
                continue;
            }
            if (symbol != '1') {
                throw std::runtime_error("Legacy .stp tap charts may contain only 0 and 1 symbols.");
            }

            notes.push_back(TapNote{
                .beat = leadInBeat + Beat(static_cast<std::int64_t>(slot), 4),
                .lane = static_cast<PanelLane>(lane),
            });
        }
    }
    return Chart(
        std::move(chartId),
        TimingMap({{{0}, beatsPerMinute}}),
        std::move(notes));
}

} // namespace pumpdx::chart
