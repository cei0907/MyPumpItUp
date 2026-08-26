#include "game/chart/TimingMap.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

constexpr double kTolerance = 0.000001;

void ExpectNear(const double actual, const double expected, const char* label) {
    if (std::abs(actual - expected) <= kTolerance) {
        return;
    }

    std::cerr << label << ": expected " << expected << ", received " << actual << '\n';
    std::exit(EXIT_FAILURE);
}

void ExpectInvalidMap(const std::vector<pumpdx::chart::TempoSegment>& segments, const char* label) {
    try {
        [[maybe_unused]] pumpdx::chart::TimingMap timingMap(segments);
    } catch (const std::invalid_argument&) {
        return;
    }

    std::cerr << label << ": expected an invalid timing map.\n";
    std::exit(EXIT_FAILURE);
}

void TestFractionNormalization() {
    const pumpdx::chart::Beat triplet(2, 6);
    ExpectNear(triplet.ToDouble(), 1.0 / 3.0, "triplet beat");
    ExpectNear((pumpdx::chart::Beat(7, 3) - pumpdx::chart::Beat(2)).ToDouble(), 1.0 / 3.0, "beat subtraction");
}

void TestSingleTempo() {
    const pumpdx::chart::TimingMap timingMap({{{0}, 120.0}});

    ExpectNear(timingMap.SecondsAt({0}), 0.0, "beat zero");
    ExpectNear(timingMap.SecondsAt({1}), 0.5, "one beat at 120 BPM");
    ExpectNear(timingMap.SecondsAt({17, 3}), 17.0 / 6.0, "triplet timing");
}

void TestTempoChange() {
    const pumpdx::chart::TimingMap timingMap({
        {{0}, 120.0},
        {{8}, 240.0},
    });

    ExpectNear(timingMap.SecondsAt({8}), 4.0, "tempo change boundary");
    ExpectNear(timingMap.SecondsAt({12}), 5.0, "tempo after change");
}

void TestInvalidMaps() {
    ExpectInvalidMap({}, "empty map");
    ExpectInvalidMap({{{1}, 120.0}}, "map without beat zero");
    ExpectInvalidMap({{{0}, 120.0}, {{0}, 180.0}}, "duplicate beat");
    ExpectInvalidMap({{{0}, 0.0}}, "zero BPM");
}

} // namespace

int main() {
    TestFractionNormalization();
    TestSingleTempo();
    TestTempoChange();
    TestInvalidMaps();

    std::cout << "TimingMap tests passed.\n";
    return EXIT_SUCCESS;
}
