#pragma once

#include "framework/assets/ThemeManifest.hpp"
#include "framework/render/GameplayFeedback.hpp"
#include "framework/render/LogicalViewport.hpp"
#include "framework/video/BgaVideoFrame.hpp"

#include <d2d1.h>
#include <d3d11.h>
#include <dwrite.h>
#include <wincodec.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

namespace pumpdx::render {

struct SceneOverlayText final {
    std::wstring_view headline;
    std::wstring_view detail;
    std::wstring_view instruction;
};

enum class SceneOverlayStyle : std::uint8_t {
    MainMenu,
    SongSelect,
    Result,
    Generic,
};

struct SceneOverlayMotion final {
    SceneOverlayStyle style = SceneOverlayStyle::Generic;
    float entrance = 1.0F;
    float loopPulse = 0.0F;
    float detailReveal = 1.0F;
    float instructionReveal = 1.0F;
};

// The renderer owns only the presentation shape. Song browsing state stays in GameFlow.
struct SongSelectOverlay final {
    std::wstring_view title;
    std::wstring_view artist;
    std::wstring_view difficultyName;
    std::wstring_view previousTitle;
    std::wstring_view nextTitle;
    std::uint8_t difficultyLevel = 0;
    std::uint32_t selectedSongNumber = 1;
    std::uint32_t songCount = 1;
    std::uint32_t selectedDifficultyNumber = 1;
    std::uint32_t difficultyCount = 1;
    bool difficultySelectionActive = false;
};

struct GameplayRenderItem final {
    std::uint8_t lane = 0;
    float headY = 0.0F;
    float holdBodyStartY = 0.0F;
    float tailY = 0.0F;
    bool isHold = false;
    bool isHoldActive = false;
    bool isHoldDamaged = false;
    bool showHead = true;
};

struct GameplayHud final {
    std::wstring_view judgement;
    std::uint32_t score = 0;
    std::uint32_t combo = 0;
    std::uint32_t maxCombo = 0;
    std::uint32_t holdTicks = 0;
    GameplayFeedback feedback;
};

class SceneOverlayRenderer final {
public:
    SceneOverlayRenderer() = default;
    ~SceneOverlayRenderer();

    SceneOverlayRenderer(const SceneOverlayRenderer&) = delete;
    SceneOverlayRenderer& operator=(const SceneOverlayRenderer&) = delete;

    [[nodiscard]] bool Initialize();
    [[nodiscard]] bool CreateTarget(ID3D11Texture2D* backBuffer);
    [[nodiscard]] bool LoadGameplayBackground(const std::filesystem::path& imagePath);
    [[nodiscard]] bool LoadGameplayVideoFrame(const video::BgaVideoFrame& frame);
    void ClearGameplayVideoFrame();
    void ReleaseTarget();
    void Shutdown();
    void Draw(
        const core::ViewportRect& viewport,
        const SceneOverlayText& text,
        const assets::ThemePalette& palette,
        const SceneOverlayMotion& motion);
    void DrawSongSelect(
        const core::ViewportRect& viewport,
        const SongSelectOverlay& song,
        const assets::ThemePalette& palette,
        const SceneOverlayMotion& motion);
    void DrawGameplay(
        const core::ViewportRect& viewport,
        const SceneOverlayText& text,
        const assets::ThemePalette& palette,
        std::span<const GameplayRenderItem> items,
        const std::array<bool, 5>& pressedPanels,
        const GameplayHud& hud,
        float energyPercent);

private:
    ID2D1Factory* factory_ = nullptr;
    ID2D1RenderTarget* target_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;
    IDWriteFactory* writeFactory_ = nullptr;
    IDWriteTextFormat* headlineFormat_ = nullptr;
    IDWriteTextFormat* detailFormat_ = nullptr;
    IDWriteTextFormat* instructionFormat_ = nullptr;
    IWICImagingFactory* wicFactory_ = nullptr;
    ID2D1PathGeometry* panelArrowGeometry_ = nullptr;
    ID2D1Bitmap* gameplayBackground_ = nullptr;
    ID2D1Bitmap* gameplayVideoFrame_ = nullptr;
    std::filesystem::path backgroundSourcePath_;
    bool backgroundLoadAttempted_ = false;
    std::uint64_t videoFrameSerial_ = 0;
    bool comInitialized_ = false;
};

} // namespace pumpdx::render
