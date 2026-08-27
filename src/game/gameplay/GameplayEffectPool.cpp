#include "game/gameplay/GameplayEffectPool.hpp"

#include <algorithm>
#include <cmath>

namespace pumpdx::gameplay {

namespace {

[[nodiscard]] float Decay(const double startedAtSeconds, const double songTimeSeconds) noexcept {
    const auto progress = static_cast<float>((songTimeSeconds - startedAtSeconds) / 0.30);
    return (std::clamp)(1.0F - progress, 0.0F, 1.0F);
}

} // namespace

void GameplayEffectPool::Trigger(const JudgementEvent& event, const double songTimeSeconds) noexcept {
    auto slot = std::find_if(effects_.begin(), effects_.end(), [songTimeSeconds](const Effect& effect) {
        return !effect.active || IsExpired(effect, songTimeSeconds);
    });
    if (slot == effects_.end()) {
        slot = std::min_element(effects_.begin(), effects_.end(), [](const Effect& left, const Effect& right) {
            return left.sequence < right.sequence;
        });
    }

    *slot = {
        .lane = event.lane,
        .judgement = event.judgement,
        .startedAtSeconds = songTimeSeconds,
        .sequence = nextSequence_++,
        .active = true,
    };
}

render::GameplayFeedback GameplayEffectPool::Sample(const double songTimeSeconds) const noexcept {
    render::GameplayFeedback feedback;
    std::uint64_t newestSequence = 0;

    for (const auto& effect : effects_) {
        if (!effect.active || IsExpired(effect, songTimeSeconds)) {
            continue;
        }

        const auto decay = Decay(effect.startedAtSeconds, songTimeSeconds);
        const auto impact = Strength(effect.judgement) * decay;
        const auto lane = static_cast<std::size_t>(effect.lane);
        if (lane < feedback.receptorImpact.size()) {
            const auto signedImpact = effect.judgement == Judgement::Miss ? -impact : impact;
            if (std::abs(signedImpact) > std::abs(feedback.receptorImpact[lane])) {
                feedback.receptorImpact[lane] = signedImpact;
            }
        }

        if (effect.sequence >= newestSequence) {
            newestSequence = effect.sequence;
            feedback.judgementBurst = impact;
            feedback.gaugeImpact = effect.judgement == Judgement::Miss ? -impact : impact;
        }
        if (effect.judgement != Judgement::Miss) {
            feedback.comboScale = (std::max)(feedback.comboScale, 1.0F + impact * 0.18F);
        }
    }

    return feedback;
}

float GameplayEffectPool::Strength(const Judgement judgement) noexcept {
    switch (judgement) {
    case Judgement::Perfect: return 1.0F;
    case Judgement::Great: return 0.88F;
    case Judgement::Good: return 0.72F;
    case Judgement::Bad: return 0.55F;
    case Judgement::Miss: return 0.90F;
    }

    return 0.0F;
}

bool GameplayEffectPool::IsExpired(const Effect& effect, const double songTimeSeconds) noexcept {
    return songTimeSeconds - effect.startedAtSeconds >= kLifetimeSeconds;
}

} // namespace pumpdx::gameplay
