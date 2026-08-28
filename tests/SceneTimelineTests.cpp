#include "framework/animation/SceneTimeline.hpp"

#include <cstdlib>
#include <iostream>

namespace {

void Expect(const bool condition, const char* message) {
    if (condition) {
        return;
    }

    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

void TestSceneElementsRevealInOrder() {
    pumpdx::animation::SceneTimeline timeline;
    timeline.Restart(10.0);

    const auto start = timeline.Sample(10.0);
    Expect(start.entrance == 0.0F && start.detailReveal == 0.0F && start.instructionReveal == 0.0F,
        "A restarted scene timeline must begin hidden.");

    const auto headlineVisible = timeline.Sample(10.40);
    Expect(headlineVisible.entrance == 1.0F && headlineVisible.detailReveal > 0.0F,
        "Headline must enter before the remaining scene text.");
    Expect(headlineVisible.instructionReveal < 1.0F,
        "Instruction text must reveal after the headline and detail.");

    const auto complete = timeline.Sample(10.70);
    Expect(complete.detailReveal == 1.0F && complete.instructionReveal == 1.0F,
        "All scene UI elements must finish revealing.");
    Expect(complete.loopPulse >= 0.0F && complete.loopPulse <= 1.0F,
        "Scene loop pulse must remain normalised.");
}

} // namespace

int main() {
    TestSceneElementsRevealInOrder();

    std::cout << "Scene timeline tests passed.\n";
    return EXIT_SUCCESS;
}
