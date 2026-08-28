#include "game/chart/NativeChartLoader.hpp"

#include <charconv>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pumpdx::chart {

namespace {

enum class Section {
    Header,
    Tempo,
    Notes,
};

[[nodiscard]] std::string Trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

[[nodiscard]] std::string Context(const std::size_t lineNumber, const std::string_view message) {
    return ".pdxchart line " + std::to_string(lineNumber) + ": " + std::string(message);
}

template <typename Integer>
[[nodiscard]] Integer ParseInteger(const std::string_view value, const std::size_t lineNumber) {
    Integer result{};
    const auto [position, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc{} || position != value.data() + value.size()) {
        throw std::runtime_error(Context(lineNumber, "expected an integer."));
    }
    return result;
}

[[nodiscard]] double ParseDouble(const std::string& value, const std::size_t lineNumber) {
    try {
        std::size_t parsedLength = 0;
        const auto result = std::stod(value, &parsedLength);
        if (parsedLength != value.size() || result <= 0.0) {
            throw std::runtime_error("invalid tempo");
        }
        return result;
    } catch (const std::exception&) {
        throw std::runtime_error(Context(lineNumber, "expected a positive BPM value."));
    }
}

[[nodiscard]] Beat ParseBeat(const std::string value, const std::size_t lineNumber) {
    const auto trimmed = Trim(value);
    if (trimmed.empty()) {
        throw std::runtime_error(Context(lineNumber, "beat cannot be empty."));
    }

    const auto plus = trimmed.find('+');
    if (plus != std::string::npos) {
        if (plus == 0 || plus == trimmed.size() - 1 || trimmed.find('+', plus + 1) != std::string::npos) {
            throw std::runtime_error(Context(lineNumber, "invalid mixed-fraction beat."));
        }
        return ParseBeat(trimmed.substr(0, plus), lineNumber) + ParseBeat(trimmed.substr(plus + 1), lineNumber);
    }

    const auto slash = trimmed.find('/');
    if (slash == std::string::npos) {
        return Beat(ParseInteger<std::int64_t>(trimmed, lineNumber));
    }
    if (slash == 0 || slash == trimmed.size() - 1 || trimmed.find('/', slash + 1) != std::string::npos) {
        throw std::runtime_error(Context(lineNumber, "invalid fractional beat."));
    }
    return Beat(
        ParseInteger<std::int64_t>(std::string_view(trimmed).substr(0, slash), lineNumber),
        ParseInteger<std::int64_t>(std::string_view(trimmed).substr(slash + 1), lineNumber));
}

[[nodiscard]] PanelLane ParseLane(const std::string_view value, const std::size_t lineNumber) {
    if (value == "SW") {
        return PanelLane::DownLeft;
    }
    if (value == "NW") {
        return PanelLane::UpLeft;
    }
    if (value == "C") {
        return PanelLane::Center;
    }
    if (value == "NE") {
        return PanelLane::UpRight;
    }
    if (value == "SE") {
        return PanelLane::DownRight;
    }
    throw std::runtime_error(Context(lineNumber, "lane must be SW, NW, C, NE, or SE."));
}

[[nodiscard]] std::vector<std::string> SplitCommaValues(const std::string& value, const std::size_t lineNumber) {
    std::vector<std::string> values;
    std::size_t begin = 0;
    while (begin <= value.size()) {
        const auto separator = value.find(',', begin);
        const auto token = Trim(value.substr(begin, separator == std::string::npos ? std::string::npos : separator - begin));
        if (token.empty()) {
            throw std::runtime_error(Context(lineNumber, "event values cannot be empty."));
        }
        values.push_back(token);
        if (separator == std::string::npos) {
            return values;
        }
        begin = separator + 1;
    }
    return values;
}

} // namespace

Chart NativeChartLoader::Load(const std::filesystem::path& sourcePath) {
    std::ifstream input(sourcePath);
    if (!input) {
        throw std::runtime_error("Unable to open .pdxchart: " + sourcePath.string());
    }

    Section section = Section::Header;
    std::string id;
    bool sawVersion = false;
    bool sawDelay = false;
    double delaySeconds = 0.0;
    std::vector<TempoSegment> tempoSegments;
    std::vector<NoteEvent> notes;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;
        line = Trim(std::move(line));
        if (line.empty() || line.starts_with('#')) {
            continue;
        }
        if (line == "[tempo]") {
            section = Section::Tempo;
            continue;
        }
        if (line == "[notes]") {
            section = Section::Notes;
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::runtime_error(Context(lineNumber, "expected key=value syntax."));
        }
        const auto key = Trim(line.substr(0, separator));
        const auto value = Trim(line.substr(separator + 1));
        if (key.empty() || value.empty()) {
            throw std::runtime_error(Context(lineNumber, "key and value must both be present."));
        }

        switch (section) {
        case Section::Header:
            if (key == "schemaVersion") {
                if (sawVersion || ParseInteger<int>(value, lineNumber) != 1) {
                    throw std::runtime_error(Context(lineNumber, "only schemaVersion=1 is supported."));
                }
                sawVersion = true;
            } else if (key == "id") {
                if (!id.empty()) {
                    throw std::runtime_error(Context(lineNumber, "chart id cannot be declared twice."));
                }
                id = value;
            } else if (key == "delayMilliseconds") {
                if (sawDelay) {
                    throw std::runtime_error(Context(lineNumber, "chart delay cannot be declared twice."));
                }
                const auto delayMilliseconds = ParseInteger<std::int64_t>(value, lineNumber);
                if (delayMilliseconds < 0) {
                    throw std::runtime_error(Context(lineNumber, "chart delay cannot be negative."));
                }
                sawDelay = true;
                delaySeconds = static_cast<double>(delayMilliseconds) / 1000.0;
            } else {
                throw std::runtime_error(Context(lineNumber, "unknown header key."));
            }
            break;
        case Section::Tempo:
            tempoSegments.push_back({.startBeat = ParseBeat(key, lineNumber), .beatsPerMinute = ParseDouble(value, lineNumber)});
            break;
        case Section::Notes: {
            const auto values = SplitCommaValues(value, lineNumber);
            if (key == "tap" && values.size() == 2) {
                notes.push_back(TapNote{.beat = ParseBeat(values[1], lineNumber), .lane = ParseLane(values[0], lineNumber)});
            } else if (key == "hold" && values.size() == 4) {
                notes.push_back(HoldNote{
                    .startBeat = ParseBeat(values[1], lineNumber),
                    .endBeat = ParseBeat(values[2], lineNumber),
                    .lane = ParseLane(values[0], lineNumber),
                    .tickPolicy = HoldTickPolicy::FixedCount(ParseInteger<std::uint32_t>(values[3], lineNumber)),
                });
            } else {
                throw std::runtime_error(Context(lineNumber, "use tap=LANE,BEAT or hold=LANE,START_BEAT,END_BEAT,TICK_COUNT."));
            }
            break;
        }
        }
    }

    if (!sawVersion || id.empty() || tempoSegments.empty() || notes.empty()) {
        throw std::runtime_error(".pdxchart requires schemaVersion, id, one tempo, and one note.");
    }
    return Chart(std::move(id), TimingMap(std::move(tempoSegments)), std::move(notes), delaySeconds);
}

} // namespace pumpdx::chart
