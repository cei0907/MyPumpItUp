#include "game/gameplay/GameplayEffectPool.hpp"

#include <cstdlib>
#include <iostream>

namespace {

using pumpdx::chart::PanelLane;
using pumpdx::gameplay::GameplayEffectPool;
using pumpdx::gameplay::Judgement;
using pumpdx::gameplay::JudgementEvent;

void Expect(const bool condition, const char* message) {
    if (condition) {
        return;
    }

    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

void TestSuccessfulJudgementProducesShortPositiveFeedback() {
    GameplayEffectPool effects;
    effects.Trigger({.lane = PanelLane::Center, .judgement = Judgement::Perfect}, 10.0);

    const auto initial = effects.Sample(10.0);
    Expect(initial.receptorImpact[2] > 0.99F, "PERFECT must flash its matching receptor.");
    Expect(initial.judgementBurst > 0.99F, "PERFECT must start a judgement burst.");
    Expect(initial.comboScale > 1.0F, "Successful judgements must pulse the combo display.");
    Expect(initial.gaugeImpact > 0.0F, "Successful judgements must create a positive gauge reaction.");

    const auto faded = effects.Sample(10.15);
    Expect(faded.receptorImpact[2] > 0.0F && faded.receptorImpact[2] < initial.receptorImpact[2],
        "Feedback must decay over audio time rather than stay frozen.");

    const auto expired = effects.Sample(10.31);
    Expect(expired.receptorImpact[2] == 0.0F && expired.judgementBurst == 0.0F,
        "Expired feedback must leave no lingering flash.");
    Expect(expired.comboScale == 1.0F && expired.gaugeImpact == 0.0F,
        "Expired feedback must return HUD values to their resting state.");
}

void TestMissUsesNegativeFeedbackAndPoolReusesSlots() {
    GameplayEffectPool effects;
    for (int index = 0; index < 40; ++index) {
        effects.Trigger({.lane = PanelLane::DownLeft, .judgement = Judgement::Miss}, 20.0 + index * 0.01);
    }

    const auto feedback = effects.Sample(20.39);
    Expect(feedback.receptorImpact[0] < 0.0F, "MISS must flash its receptor using negative feedback.");
    Expect(feedback.gaugeImpact < 0.0F, "MISS must create a negative gauge reaction.");
    Expect(feedback.comboScale == 1.0F, "MISS must not trigger a combo pulse.");
}

} // namespace

int main() {
    TestSuccessfulJudgementProducesShortPositiveFeedback();
    TestMissUsesNegativeFeedbackAndPoolReusesSlots();

    std::cout << "Gameplay effect-pool tests passed.\n";
    return EXIT_SUCCESS;
}
