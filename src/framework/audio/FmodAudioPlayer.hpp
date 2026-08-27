#pragma once

#include <filesystem>
#include <memory>

namespace pumpdx::audio {

class FmodAudioPlayer final {
public:
    FmodAudioPlayer();
    ~FmodAudioPlayer();

    FmodAudioPlayer(const FmodAudioPlayer&) = delete;
    FmodAudioPlayer& operator=(const FmodAudioPlayer&) = delete;

    [[nodiscard]] bool Initialize();
    [[nodiscard]] bool Play(const std::filesystem::path& audioFilePath);
    void Stop() noexcept;
    void Update() noexcept;

    [[nodiscard]] double PlaybackSeconds() const noexcept;
    [[nodiscard]] bool IsAvailable() const noexcept;
    [[nodiscard]] bool IsPlaying() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace pumpdx::audio
