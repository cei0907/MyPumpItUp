#include "core/LogicalViewport.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

constexpr float kTolerance = 0.001F;

void ExpectNear(const float actual, const float expected, const char* label) {
    if (std::abs(actual - expected) <= kTolerance) {
        return;
    }

    std::cerr << label << ": expected " << expected << ", received " << actual << '\n';
    std::exit(EXIT_FAILURE);
}

void TestExactDesignResolution() {
    const auto viewport = pumpdx::core::LogicalViewport::FitInside(1280, 720);

    ExpectNear(viewport.x, 0.0F, "exact x");
    ExpectNear(viewport.y, 0.0F, "exact y");
    ExpectNear(viewport.width, 1280.0F, "exact width");
    ExpectNear(viewport.height, 720.0F, "exact height");
    ExpectNear(viewport.scale, 1.0F, "exact scale");
}

void TestWidescreenLetterboxing() {
    const auto viewport = pumpdx::core::LogicalViewport::FitInside(3440, 1440);

    ExpectNear(viewport.x, 440.0F, "wide x");
    ExpectNear(viewport.y, 0.0F, "wide y");
    ExpectNear(viewport.width, 2560.0F, "wide width");
    ExpectNear(viewport.height, 1440.0F, "wide height");
    ExpectNear(viewport.scale, 2.0F, "wide scale");
}

void TestTallLetterboxing() {
    const auto viewport = pumpdx::core::LogicalViewport::FitInside(1280, 1024);

    ExpectNear(viewport.x, 0.0F, "tall x");
    ExpectNear(viewport.y, 152.0F, "tall y");
    ExpectNear(viewport.width, 1280.0F, "tall width");
    ExpectNear(viewport.height, 720.0F, "tall height");
    ExpectNear(viewport.scale, 1.0F, "tall scale");
}

void TestZeroOutputIsEmpty() {
    const auto viewport = pumpdx::core::LogicalViewport::FitInside(0, 720);

    ExpectNear(viewport.width, 0.0F, "zero width");
    ExpectNear(viewport.height, 0.0F, "zero height");
    ExpectNear(viewport.scale, 0.0F, "zero scale");
}

} // namespace

int main() {
    TestExactDesignResolution();
    TestWidescreenLetterboxing();
    TestTallLetterboxing();
    TestZeroOutputIsEmpty();

    std::cout << "LogicalViewport tests passed.\n";
    return EXIT_SUCCESS;
}
