#pragma once

#include "framework/video/BgaVideoFrame.hpp"

#include <filesystem>

struct IMFSourceReader;

namespace pumpdx::video {

class MediaFoundationBgaPlayer final {
public:
    MediaFoundationBgaPlayer() = default;
    ~MediaFoundationBgaPlayer();

    MediaFoundationBgaPlayer(const MediaFoundationBgaPlayer&) = delete;
    MediaFoundationBgaPlayer& operator=(const MediaFoundationBgaPlayer&) = delete;

    [[nodiscard]] bool Open(const std::filesystem::path& videoPath);
    [[nodiscard]] const BgaVideoFrame* FrameAt(double audioTimeSeconds);
    void Close() noexcept;

private:
    [[nodiscard]] bool ConfigureReader();
    [[nodiscard]] bool Seek(double audioTimeSeconds);
    [[nodiscard]] bool DecodeNextPendingFrame();
    [[nodiscard]] bool CopySampleToFrame(void* sample, BgaVideoFrame& output) const;

    IMFSourceReader* reader_ = nullptr;
    std::filesystem::path sourcePath_;
    BgaVideoFrame currentFrame_;
    BgaVideoFrame pendingFrame_;
    std::int64_t pendingTimestampHns_ = 0;
    std::uint64_t nextFrameSerial_ = 1;
    std::uint32_t frameWidth_ = 0;
    std::uint32_t frameHeight_ = 0;
    double lastAudioTimeSeconds_ = 0.0;
    bool hasCurrentFrame_ = false;
    bool hasPendingFrame_ = false;
    bool sourceReachedEnd_ = false;
    bool openAttempted_ = false;
    bool mediaFoundationStarted_ = false;
};

} // namespace pumpdx::video
