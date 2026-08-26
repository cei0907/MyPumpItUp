#include "game/gameplay/Judgement.hpp"
#include "game/gameplay/ScoreState.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

using pumpdx::chart::PanelLane;
using pumpdx::gameplay::JudgableNote;
using pumpdx::gameplay::Judgement;
using pumpdx::gameplay::JudgementEngine;
using pumpdx::gameplay::JudgementEvent;

void Expect(const bool condition, const char* message) {
    if (condition) {
        return;
    }

    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

void TestJudgementWindowsAreTimeBasedAndInclusive() {
    JudgementEngine engine({{.lane = PanelLane::Center, .timeSeconds = 10.0}});
    const auto event = engine.TryJudge(PanelLane::Center, 10.060);

    Expect(event.has_value(), "Input on the judgement-window boundary must be accepted.");
    Expect(event->judgement == Judgement::Great, "The 60 ms boundary must classify as GREAT.");
    Expect(engine.IsResolved(0), "A judged note must not remain active.");
}

void TestWrongLaneDoesNotConsumeNote() {
    JudgementEngine engine({{.lane = PanelLane::UpLeft, .timeSeconds = 2.0}});
    Expect(!engine.TryJudge(PanelLane::Center, 2.0).has_value(), "Wrong-lane input must not judge another lane.");
    Expect(!engine.IsResolved(0), "Wrong-lane input must not consume the note.");
}

void TestAutomaticMissAndComboReset() {
    JudgementEngine engine({
        {.lane = PanelLane::DownLeft, .timeSeconds = 1.0},
        {.lane = PanelLane::Center, .timeSeconds = 2.0},
    });
    pumpdx::gameplay::ScoreState score;

    const auto perfect = engine.TryJudge(PanelLane::DownLeft, 1.0);
    Expect(perfect.has_value(), "Exact input must judge the note.");
    score.Apply(*perfect);

    const auto misses = engine.CollectMisses(2.151);
    Expect(misses.size() == 1 && misses.front().judgement == Judgement::Miss, "Unhit note must become an automatic MISS after 150 ms.");
    score.Apply(misses.front());

    Expect(score.Score() == 1000, "MISS must not add score.");
    Expect(score.MaxCombo() == 1, "Max combo must preserve the best streak.");
    Expect(score.CurrentCombo() == 0, "MISS must reset the current combo.");
    Expect(score.BuildSummary().judgedNotes == 2, "Every judgement and automatic miss must be counted.");
}

} // namespace

int main() {
    TestJudgementWindowsAreTimeBasedAndInclusive();
    TestWrongLaneDoesNotConsumeNote();
    TestAutomaticMissAndComboReset();

    std::cout << "Judgement tests passed.\n";
    return EXIT_SUCCESS;
}
