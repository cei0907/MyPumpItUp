#include "framework/video/MediaFoundationBgaPlayer.hpp"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace pumpdx::video {

namespace {

constexpr std::int64_t kHundredNanosecondsPerSecond = 10'000'000;
constexpr double kBackwardSeekThresholdSeconds = 0.10;
constexpr std::size_t kMaximumDecodedFramesPerCall = 24;
constexpr DWORD kAllSourceReaderStreams = static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS);
constexpr DWORD kFirstVideoSourceReaderStream = static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM);

template <typename T>
void ReleaseCom(T*& object) noexcept {
    if (object != nullptr) {
        object->Release();
        object = nullptr;
    }
}

[[nodiscard]] std::int64_t ToHundredNanoseconds(const double seconds) noexcept {
    const auto clampedSeconds = (std::max)(seconds, 0.0);
    const auto scaled = clampedSeconds * static_cast<double>(kHundredNanosecondsPerSecond);
    return scaled >= static_cast<double>((std::numeric_limits<std::int64_t>::max)())
        ? (std::numeric_limits<std::int64_t>::max)()
        : static_cast<std::int64_t>(std::llround(scaled));
}

} // namespace

MediaFoundationBgaPlayer::~MediaFoundationBgaPlayer() {
    Close();
}

bool MediaFoundationBgaPlayer::Open(const std::filesystem::path& videoPath) {
    if (videoPath == sourcePath_ && openAttempted_) {
        return reader_ != nullptr;
    }

    Close();
    sourcePath_ = videoPath;
    openAttempted_ = true;
    if (videoPath.empty() || !std::filesystem::is_regular_file(videoPath)) {
        return false;
    }

    if (FAILED(MFStartup(MF_VERSION))) {
        return false;
    }
    mediaFoundationStarted_ = true;

    IMFAttributes* attributes = nullptr;
    const auto attributesResult = MFCreateAttributes(&attributes, 1);
    const auto videoProcessingResult = SUCCEEDED(attributesResult)
        ? attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE)
        : E_FAIL;
    const auto createReaderResult = SUCCEEDED(videoProcessingResult)
        ? MFCreateSourceReaderFromURL(videoPath.c_str(), attributes, &reader_)
        : E_FAIL;
    ReleaseCom(attributes);
    if (FAILED(createReaderResult) || !ConfigureReader()) {
        Close();
        sourcePath_ = videoPath;
        openAttempted_ = true;
        return false;
    }

    return true;
}

const BgaVideoFrame* MediaFoundationBgaPlayer::FrameAt(const double audioTimeSeconds) {
    if (reader_ == nullptr) {
        return nullptr;
    }

    const auto targetTimeSeconds = (std::max)(audioTimeSeconds, 0.0);
    if (targetTimeSeconds + kBackwardSeekThresholdSeconds < lastAudioTimeSeconds_ && !Seek(targetTimeSeconds)) {
        return hasCurrentFrame_ ? &currentFrame_ : nullptr;
    }
    lastAudioTimeSeconds_ = targetTimeSeconds;
    const auto targetTimestampHns = ToHundredNanoseconds(targetTimeSeconds);

    for (std::size_t decoded = 0; decoded < kMaximumDecodedFramesPerCall; ++decoded) {
        if (!hasPendingFrame_ && !DecodeNextPendingFrame()) {
            break;
        }
        if (!hasPendingFrame_ || pendingTimestampHns_ > targetTimestampHns) {
            break;
        }

        std::swap(currentFrame_, pendingFrame_);
        currentFrame_.serial = nextFrameSerial_++;
        hasCurrentFrame_ = true;
        hasPendingFrame_ = false;
    }

    return hasCurrentFrame_ ? &currentFrame_ : nullptr;
}

void MediaFoundationBgaPlayer::Close() noexcept {
    ReleaseCom(reader_);
    if (mediaFoundationStarted_) {
        static_cast<void>(MFShutdown());
        mediaFoundationStarted_ = false;
    }
    sourcePath_.clear();
    currentFrame_ = {};
    pendingFrame_ = {};
    pendingTimestampHns_ = 0;
    nextFrameSerial_ = 1;
    frameWidth_ = 0;
    frameHeight_ = 0;
    lastAudioTimeSeconds_ = 0.0;
    hasCurrentFrame_ = false;
    hasPendingFrame_ = false;
    sourceReachedEnd_ = false;
    openAttempted_ = false;
}

bool MediaFoundationBgaPlayer::ConfigureReader() {
    if (reader_ == nullptr) {
        return false;
    }

    if (FAILED(reader_->SetStreamSelection(kAllSourceReaderStreams, FALSE))
        || FAILED(reader_->SetStreamSelection(kFirstVideoSourceReaderStream, TRUE))) {
        return false;
    }

    IMFMediaType* outputType = nullptr;
    const auto createTypeResult = MFCreateMediaType(&outputType);
    const auto majorTypeResult = SUCCEEDED(createTypeResult)
        ? outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video)
        : E_FAIL;
    const auto subtypeResult = SUCCEEDED(majorTypeResult)
        ? outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32)
        : E_FAIL;
    const auto setTypeResult = SUCCEEDED(subtypeResult)
        ? reader_->SetCurrentMediaType(kFirstVideoSourceReaderStream, nullptr, outputType)
        : E_FAIL;
    ReleaseCom(outputType);
    if (FAILED(setTypeResult)) {
        return false;
    }

    IMFMediaType* configuredType = nullptr;
    const auto getTypeResult = reader_->GetCurrentMediaType(kFirstVideoSourceReaderStream, &configuredType);
    const auto sizeResult = SUCCEEDED(getTypeResult)
        ? MFGetAttributeSize(configuredType, MF_MT_FRAME_SIZE, &frameWidth_, &frameHeight_)
        : E_FAIL;
    ReleaseCom(configuredType);
    return SUCCEEDED(sizeResult) && frameWidth_ > 0 && frameHeight_ > 0;
}

bool MediaFoundationBgaPlayer::Seek(const double audioTimeSeconds) {
    if (reader_ == nullptr) {
        return false;
    }

    PROPVARIANT position{};
    PropVariantInit(&position);
    position.vt = VT_I8;
    position.hVal.QuadPart = ToHundredNanoseconds(audioTimeSeconds);
    const auto result = reader_->SetCurrentPosition(GUID_NULL, position);
    PropVariantClear(&position);
    if (FAILED(result)) {
        return false;
    }

    currentFrame_ = {};
    pendingFrame_ = {};
    pendingTimestampHns_ = 0;
    hasCurrentFrame_ = false;
    hasPendingFrame_ = false;
    sourceReachedEnd_ = false;
    return true;
}

bool MediaFoundationBgaPlayer::DecodeNextPendingFrame() {
    if (reader_ == nullptr || sourceReachedEnd_) {
        return false;
    }

    DWORD streamIndex = 0;
    DWORD streamFlags = 0;
    LONGLONG timestampHns = 0;
    IMFSample* sample = nullptr;
    const auto readResult = reader_->ReadSample(
        kFirstVideoSourceReaderStream,
        0,
        &streamIndex,
        &streamFlags,
        &timestampHns,
        &sample);

    if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
        sourceReachedEnd_ = true;
    }
    const auto copied = SUCCEEDED(readResult) && sample != nullptr && CopySampleToFrame(sample, pendingFrame_);
    ReleaseCom(sample);
    if (!copied) {
        return false;
    }

    pendingTimestampHns_ = timestampHns;
    hasPendingFrame_ = true;
    return true;
}

bool MediaFoundationBgaPlayer::CopySampleToFrame(void* const rawSample, BgaVideoFrame& output) const {
    auto* const sample = static_cast<IMFSample*>(rawSample);
    if (reader_ == nullptr || sample == nullptr) {
        return false;
    }

    if (frameWidth_ == 0 || frameHeight_ == 0) {
        return false;
    }

    IMFMediaBuffer* buffer = nullptr;
    if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) {
        return false;
    }

    BYTE* source = nullptr;
    DWORD maximumLength = 0;
    DWORD currentLength = 0;
    const auto lockResult = buffer->Lock(&source, &maximumLength, &currentLength);
    const auto expectedLength = static_cast<std::size_t>(frameWidth_) * static_cast<std::size_t>(frameHeight_) * 4U;
    const auto valid = SUCCEEDED(lockResult) && source != nullptr && currentLength >= expectedLength;
    if (valid) {
        output.width = frameWidth_;
        output.height = frameHeight_;
        output.bgraPixels.assign(source, source + expectedLength);
        for (std::size_t index = 3; index < output.bgraPixels.size(); index += 4) {
            output.bgraPixels[index] = 255;
        }
    }
    if (SUCCEEDED(lockResult)) {
        static_cast<void>(buffer->Unlock());
    }
    ReleaseCom(buffer);
    return valid;
}

} // namespace pumpdx::video
