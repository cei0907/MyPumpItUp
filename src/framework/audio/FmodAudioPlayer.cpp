#include "framework/audio/FmodAudioPlayer.hpp"

#ifdef PUMP_DX_HAS_FMOD
#include <fmod.hpp>
#endif

#include <utility>

namespace pumpdx::audio {

struct FmodAudioPlayer::Impl final {
#ifdef PUMP_DX_HAS_FMOD
    FMOD::System* system = nullptr;
    FMOD::Sound* sound = nullptr;
    FMOD::Channel* channel = nullptr;
#endif
    bool available = false;
    double lastPlaybackSeconds = 0.0;
};

FmodAudioPlayer::FmodAudioPlayer()
    : impl_(std::make_unique<Impl>()) {
}

FmodAudioPlayer::~FmodAudioPlayer() {
    Stop();
#ifdef PUMP_DX_HAS_FMOD
    if (impl_->system != nullptr) {
        impl_->system->close();
        impl_->system->release();
    }
#endif
}

bool FmodAudioPlayer::Initialize() {
#ifdef PUMP_DX_HAS_FMOD
    if (impl_->available) {
        return true;
    }
    if (FMOD::System_Create(&impl_->system) != FMOD_OK) {
        return false;
    }
    if (impl_->system->init(64, FMOD_INIT_NORMAL, nullptr) != FMOD_OK) {
        impl_->system->release();
        impl_->system = nullptr;
        return false;
    }
    impl_->available = true;
    return true;
#else
    return false;
#endif
}

bool FmodAudioPlayer::Play(const std::filesystem::path& audioFilePath) {
#ifdef PUMP_DX_HAS_FMOD
    if (!impl_->available || audioFilePath.empty() || !std::filesystem::is_regular_file(audioFilePath)) {
        return false;
    }

    Stop();
    const auto path = audioFilePath.string();
    if (impl_->system->createStream(path.c_str(), FMOD_DEFAULT, nullptr, &impl_->sound) != FMOD_OK) {
        impl_->sound = nullptr;
        return false;
    }
    if (impl_->system->playSound(impl_->sound, nullptr, false, &impl_->channel) != FMOD_OK) {
        impl_->sound->release();
        impl_->sound = nullptr;
        impl_->channel = nullptr;
        return false;
    }
    impl_->lastPlaybackSeconds = 0.0;
    return true;
#else
    static_cast<void>(audioFilePath);
    return false;
#endif
}

void FmodAudioPlayer::Stop() noexcept {
#ifdef PUMP_DX_HAS_FMOD
    if (impl_->channel != nullptr) {
        impl_->channel->stop();
        impl_->channel = nullptr;
    }
    if (impl_->sound != nullptr) {
        impl_->sound->release();
        impl_->sound = nullptr;
    }
#endif
    impl_->lastPlaybackSeconds = 0.0;
}

void FmodAudioPlayer::Update() noexcept {
#ifdef PUMP_DX_HAS_FMOD
    if (impl_->system != nullptr) {
        impl_->system->update();
    }
#endif
}

double FmodAudioPlayer::PlaybackSeconds() const noexcept {
#ifdef PUMP_DX_HAS_FMOD
    if (impl_->channel != nullptr) {
        unsigned int milliseconds = 0;
        if (impl_->channel->getPosition(&milliseconds, FMOD_TIMEUNIT_MS) == FMOD_OK) {
            impl_->lastPlaybackSeconds = static_cast<double>(milliseconds) / 1000.0;
        }
    }
#endif
    return impl_->lastPlaybackSeconds;
}

bool FmodAudioPlayer::IsAvailable() const noexcept {
    return impl_->available;
}

bool FmodAudioPlayer::IsPlaying() const noexcept {
#ifdef PUMP_DX_HAS_FMOD
    bool playing = false;
    if (impl_->channel != nullptr) {
        impl_->channel->isPlaying(&playing);
    }
    return playing;
#else
    return false;
#endif
}

} // namespace pumpdx::audio
