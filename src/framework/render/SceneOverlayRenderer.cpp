#include "framework/render/SceneOverlayRenderer.hpp"

#include "framework/render/GameplayLayout.hpp"

#include <d2d1helper.h>
#include <dxgi.h>
#include <objbase.h>
#include <wincodec.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace pumpdx::render {

namespace {

[[nodiscard]] D2D1_COLOR_F ToD2DColor(const std::array<float, 4>& color) {
    return {color[0], color[1], color[2], color[3]};
}

[[nodiscard]] D2D1_COLOR_F WithOpacity(D2D1_COLOR_F color, const float opacity) {
    color.a *= (std::clamp)(opacity, 0.0F, 1.0F);
    return color;
}

template <typename T>
void ReleaseCom(T*& object) {
    if (object != nullptr) {
        object->Release();
        object = nullptr;
    }
}

[[nodiscard]] bool CreatePanelArrowGeometry(ID2D1Factory* const factory, ID2D1PathGeometry** const output) {
    if (factory == nullptr || output == nullptr) {
        return false;
    }

    ID2D1PathGeometry* geometry = nullptr;
    if (FAILED(factory->CreatePathGeometry(&geometry))) {
        return false;
    }

    ID2D1GeometrySink* sink = nullptr;
    if (FAILED(geometry->Open(&sink))) {
        ReleaseCom(geometry);
        return false;
    }

    const std::array points{
        D2D1::Point2F(0.0F, -44.0F),
        D2D1::Point2F(42.0F, -2.0F),
        D2D1::Point2F(19.0F, -2.0F),
        D2D1::Point2F(19.0F, 42.0F),
        D2D1::Point2F(-19.0F, 42.0F),
        D2D1::Point2F(-19.0F, -2.0F),
        D2D1::Point2F(-42.0F, -2.0F),
    };
    sink->BeginFigure(points.front(), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLines(points.data() + 1, static_cast<UINT32>(points.size() - 1));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    const auto closeResult = sink->Close();
    ReleaseCom(sink);
    if (FAILED(closeResult)) {
        ReleaseCom(geometry);
        return false;
    }

    *output = geometry;
    return true;
}

[[nodiscard]] float LaneArrowRotationDegrees(const std::uint8_t lane) {
    constexpr std::array<float, 5> rotations{-135.0F, -45.0F, 0.0F, 45.0F, 135.0F};
    return lane < rotations.size() ? rotations[lane] : 0.0F;
}

void DrawArrowGlyph(
    ID2D1RenderTarget* const target,
    ID2D1SolidColorBrush* const brush,
    ID2D1PathGeometry* const geometry,
    const D2D1_MATRIX_3X2_F& parentTransform,
    const float centerX,
    const float centerY,
    const float scale,
    const float rotationDegrees,
    const D2D1_COLOR_F& fillColor,
    const D2D1_COLOR_F& outlineColor,
    const float outlineWidth) {
    if (target == nullptr || brush == nullptr || geometry == nullptr) {
        return;
    }

    target->SetTransform(
        parentTransform
        * D2D1::Matrix3x2F::Scale(scale, scale)
        * D2D1::Matrix3x2F::Rotation(rotationDegrees)
        * D2D1::Matrix3x2F::Translation(centerX, centerY));
    brush->SetColor(fillColor);
    target->FillGeometry(geometry, brush);
    brush->SetColor(outlineColor);
    target->DrawGeometry(geometry, brush, outlineWidth / scale);
    target->SetTransform(parentTransform);
}

void DrawPanelGlyph(
    ID2D1RenderTarget* const target,
    ID2D1SolidColorBrush* const brush,
    ID2D1PathGeometry* const geometry,
    const D2D1_MATRIX_3X2_F& parentTransform,
    const std::uint8_t lane,
    const float centerX,
    const float centerY,
    const float scale,
    const D2D1_COLOR_F& fillColor,
    const D2D1_COLOR_F& outlineColor,
    const float outlineWidth) {
    if (lane != 2) {
        DrawArrowGlyph(target, brush, geometry, parentTransform, centerX, centerY, scale,
            LaneArrowRotationDegrees(lane), fillColor, outlineColor, outlineWidth);
        return;
    }

    constexpr std::array<float, 4> rotations{0.0F, 90.0F, 180.0F, 270.0F};
    const std::array<D2D1_POINT_2F, 4> offsets{
        D2D1::Point2F(0.0F, -15.0F),
        D2D1::Point2F(15.0F, 0.0F),
        D2D1::Point2F(0.0F, 15.0F),
        D2D1::Point2F(-15.0F, 0.0F),
    };
    for (std::size_t index = 0; index < rotations.size(); ++index) {
        DrawArrowGlyph(target, brush, geometry, parentTransform,
            centerX + offsets[index].x * scale, centerY + offsets[index].y * scale, scale * 0.40F,
            rotations[index], fillColor, outlineColor, outlineWidth);
    }
}

} // namespace

SceneOverlayRenderer::~SceneOverlayRenderer() {
    Shutdown();
}

bool SceneOverlayRenderer::Initialize() {
    const auto comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    comInitialized_ = SUCCEEDED(comResult);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        return false;
    }

    const auto factoryResult = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory_);
    if (FAILED(factoryResult)) {
        return false;
    }

    const auto writeFactoryResult = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&writeFactory_));
    if (FAILED(writeFactoryResult)) {
        Shutdown();
        return false;
    }

    const auto wicResult = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory_));
    if (FAILED(wicResult)) {
        Shutdown();
        return false;
    }

    const auto createHeadlineResult = writeFactory_->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 62.0F, L"ko-KR", &headlineFormat_);
    const auto createDetailResult = writeFactory_->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 28.0F, L"ko-KR", &detailFormat_);
    const auto createInstructionResult = writeFactory_->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 22.0F, L"ko-KR", &instructionFormat_);

    if (FAILED(createHeadlineResult) || FAILED(createDetailResult) || FAILED(createInstructionResult)) {
        Shutdown();
        return false;
    }

    if (!CreatePanelArrowGeometry(factory_, &panelArrowGeometry_)) {
        Shutdown();
        return false;
    }

    return true;
}

bool SceneOverlayRenderer::CreateTarget(ID3D11Texture2D* const backBuffer) {
    ReleaseTarget();

    if (factory_ == nullptr || backBuffer == nullptr) {
        return false;
    }

    IDXGISurface* surface = nullptr;
    const auto queryResult = backBuffer->QueryInterface(IID_PPV_ARGS(&surface));
    if (FAILED(queryResult)) {
        return false;
    }

    const auto properties = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
    const auto targetResult = factory_->CreateDxgiSurfaceRenderTarget(surface, &properties, &target_);
    surface->Release();
    if (FAILED(targetResult)) {
        return false;
    }

    const auto brushResult = target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush_);
    if (FAILED(brushResult)) {
        ReleaseTarget();
        return false;
    }

    return true;
}

bool SceneOverlayRenderer::LoadGameplayBackground(const std::filesystem::path& imagePath) {
    if (target_ == nullptr || wicFactory_ == nullptr) {
        return false;
    }
    if (imagePath == backgroundSourcePath_ && backgroundLoadAttempted_) {
        return gameplayBackground_ != nullptr;
    }

    ReleaseCom(gameplayBackground_);
    backgroundSourcePath_ = imagePath;
    backgroundLoadAttempted_ = true;
    if (imagePath.empty() || !std::filesystem::is_regular_file(imagePath)) {
        return false;
    }

    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;
    const auto decoderResult = wicFactory_->CreateDecoderFromFilename(
        imagePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    const auto frameResult = SUCCEEDED(decoderResult) ? decoder->GetFrame(0, &frame) : E_FAIL;
    const auto converterResult = SUCCEEDED(frameResult) ? wicFactory_->CreateFormatConverter(&converter) : E_FAIL;
    const auto initializeResult = SUCCEEDED(converterResult)
        ? converter->Initialize(
            frame,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom)
        : E_FAIL;
    const auto bitmapResult = SUCCEEDED(initializeResult)
        ? target_->CreateBitmapFromWicBitmap(converter, nullptr, &gameplayBackground_)
        : E_FAIL;

    ReleaseCom(converter);
    ReleaseCom(frame);
    ReleaseCom(decoder);
    return SUCCEEDED(bitmapResult);
}

bool SceneOverlayRenderer::LoadGameplayVideoFrame(const video::BgaVideoFrame& frame) {
    if (target_ == nullptr || frame.width == 0 || frame.height == 0 || frame.serial == 0
        || frame.bgraPixels.size() != static_cast<std::size_t>(frame.width) * frame.height * 4U) {
        return false;
    }
    if (frame.serial == videoFrameSerial_ && gameplayVideoFrame_ != nullptr) {
        return true;
    }

    ID2D1Bitmap* bitmap = nullptr;
    const auto result = target_->CreateBitmap(
        D2D1::SizeU(frame.width, frame.height),
        frame.bgraPixels.data(),
        frame.width * 4U,
        D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
        &bitmap);
    if (FAILED(result)) {
        return false;
    }

    ReleaseCom(gameplayVideoFrame_);
    gameplayVideoFrame_ = bitmap;
    videoFrameSerial_ = frame.serial;
    return true;
}

void SceneOverlayRenderer::ClearGameplayVideoFrame() {
    ReleaseCom(gameplayVideoFrame_);
    videoFrameSerial_ = 0;
}

void SceneOverlayRenderer::ReleaseTarget() {
    ClearGameplayVideoFrame();
    ReleaseCom(gameplayBackground_);
    backgroundLoadAttempted_ = false;
    ReleaseCom(brush_);
    ReleaseCom(target_);
}

void SceneOverlayRenderer::Shutdown() {
    ReleaseTarget();
    ReleaseCom(panelArrowGeometry_);
    ReleaseCom(instructionFormat_);
    ReleaseCom(detailFormat_);
    ReleaseCom(headlineFormat_);
    ReleaseCom(writeFactory_);
    ReleaseCom(factory_);
    ReleaseCom(wicFactory_);
    if (comInitialized_) {
        CoUninitialize();
        comInitialized_ = false;
    }
}

void SceneOverlayRenderer::Draw(
    const core::ViewportRect& viewport,
    const SceneOverlayText& text,
    const assets::ThemePalette& palette,
    const SceneOverlayMotion& motion) {
    if (target_ == nullptr || brush_ == nullptr || viewport.scale <= 0.0F) {
        return;
    }

    target_->BeginDraw();
    target_->SetTransform(
        D2D1::Matrix3x2F::Scale(viewport.scale, viewport.scale)
        * D2D1::Matrix3x2F::Translation(viewport.x, viewport.y));

    const auto entrance = (std::clamp)(motion.entrance, 0.0F, 1.0F);
    const auto cardTop = 112.0F + (1.0F - entrance) * 72.0F;
    const auto cardBottom = 608.0F + (1.0F - entrance) * 72.0F;
    const auto pulse = (std::clamp)(motion.loopPulse, 0.0F, 1.0F);

    brush_->SetColor(WithOpacity(ToD2DColor(palette.panel), 0.55F + entrance * 0.45F));
    target_->FillRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(106.0F, cardTop, 1174.0F, cardBottom), 28.0F, 28.0F), brush_);

    brush_->SetColor(WithOpacity(ToD2DColor(palette.accent), entrance));
    target_->FillRectangle(D2D1::RectF(156.0F, cardTop + 85.0F, 166.0F, cardTop + 326.0F), brush_);

    if (motion.style == SceneOverlayStyle::MainMenu) {
        brush_->SetColor(WithOpacity(ToD2DColor(palette.accent), 0.08F + pulse * 0.10F));
        target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(1055.0F, cardTop + 74.0F), 126.0F + pulse * 36.0F, 126.0F + pulse * 36.0F), brush_);
    } else if (motion.style == SceneOverlayStyle::SongSelect) {
        brush_->SetColor(WithOpacity(ToD2DColor(palette.accent), 0.18F + pulse * 0.22F));
        target_->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(184.0F, cardTop + 274.0F, 1098.0F, cardTop + 334.0F), 14.0F, 14.0F), brush_);
    } else if (motion.style == SceneOverlayStyle::Result) {
        brush_->SetColor(WithOpacity(ToD2DColor(palette.accent), 0.12F + pulse * 0.14F));
        target_->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(202.0F, cardTop + 294.0F, 1078.0F, cardTop + 304.0F), 5.0F, 5.0F), brush_);
    }

    brush_->SetColor(WithOpacity(ToD2DColor(palette.heading), entrance));
    target_->DrawText(
        text.headline.data(), static_cast<UINT32>(text.headline.size()), headlineFormat_,
        D2D1::RectF(204.0F, cardTop + 58.0F, 1100.0F, cardTop + 148.0F), brush_);

    brush_->SetColor(WithOpacity(ToD2DColor(palette.detail), motion.detailReveal));
    target_->DrawText(
        text.detail.data(), static_cast<UINT32>(text.detail.size()), detailFormat_,
        D2D1::RectF(204.0F, cardTop + 182.0F, 1080.0F, cardTop + 248.0F), brush_);

    brush_->SetColor(WithOpacity(ToD2DColor(palette.instruction), motion.instructionReveal));
    target_->DrawText(
        text.instruction.data(), static_cast<UINT32>(text.instruction.size()), instructionFormat_,
        D2D1::RectF(204.0F, cardTop + 356.0F, 1080.0F, cardTop + 414.0F), brush_);

    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->EndDraw();
}

void SceneOverlayRenderer::DrawSongSelect(
    const core::ViewportRect& viewport,
    const SongSelectOverlay& song,
    const assets::ThemePalette& palette,
    const SceneOverlayMotion& motion) {
    if (target_ == nullptr || brush_ == nullptr || viewport.scale <= 0.0F) {
        return;
    }

    target_->BeginDraw();
    const auto transform = D2D1::Matrix3x2F::Scale(viewport.scale, viewport.scale)
        * D2D1::Matrix3x2F::Translation(viewport.x, viewport.y);
    target_->SetTransform(transform);

    const auto entrance = (std::clamp)(motion.entrance, 0.0F, 1.0F);
    const auto pulse = (std::clamp)(motion.loopPulse, 0.0F, 1.0F);
    const auto heading = ToD2DColor(palette.heading);
    const auto detail = ToD2DColor(palette.detail);
    const auto accent = ToD2DColor(palette.accent);
    const auto panel = ToD2DColor(palette.panel);

    const auto drawText = [this](
                              const std::wstring_view value,
                              IDWriteTextFormat* const format,
                              const D2D1_RECT_F rect,
                              const D2D1_COLOR_F color) {
        brush_->SetColor(color);
        target_->DrawText(value.data(), static_cast<UINT32>(value.size()), format, rect, brush_);
    };
    const auto drawSongCard = [this, &drawText, panel, heading, detail, accent, entrance](
                                  const D2D1_RECT_F rect,
                                  const std::wstring_view caption,
                                  const std::wstring_view value,
                                  const bool selected) {
        brush_->SetColor(WithOpacity(panel, selected ? 0.92F * entrance : 0.50F * entrance));
        target_->FillRoundedRectangle(D2D1::RoundedRect(rect, 16.0F, 16.0F), brush_);
        brush_->SetColor(WithOpacity(selected ? accent : detail, selected ? entrance : 0.40F * entrance));
        target_->DrawRoundedRectangle(D2D1::RoundedRect(rect, 16.0F, 16.0F), brush_, selected ? 4.0F : 2.0F);
        drawText(caption, instructionFormat_, D2D1::RectF(rect.left + 18.0F, rect.top + 14.0F, rect.right - 18.0F, rect.top + 45.0F),
            WithOpacity(detail, selected ? entrance : 0.55F * entrance));
        drawText(value, selected ? headlineFormat_ : detailFormat_,
            D2D1::RectF(rect.left + 20.0F, rect.top + 48.0F, rect.right - 20.0F, rect.bottom - 12.0F),
            WithOpacity(heading, selected ? entrance : 0.72F * entrance));
    };

    brush_->SetColor(WithOpacity(panel, 0.82F));
    target_->FillRectangle(D2D1::RectF(0.0F, 0.0F, 1280.0F, 720.0F), brush_);
    brush_->SetColor(WithOpacity(accent, 0.08F + pulse * 0.08F));
    target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(640.0F, 228.0F), 380.0F + pulse * 24.0F, 226.0F + pulse * 14.0F), brush_);

    drawText(L"SONG SELECT", detailFormat_, D2D1::RectF(62.0F, 42.0F, 350.0F, 78.0F), WithOpacity(heading, entrance));
    drawText(std::to_wstring(song.selectedSongNumber) + L" / " + std::to_wstring(song.songCount), instructionFormat_,
        D2D1::RectF(1065.0F, 46.0F, 1214.0F, 78.0F), WithOpacity(detail, entrance));

    brush_->SetColor(WithOpacity(panel, 0.88F * entrance));
    target_->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(52.0F, 126.0F, 272.0F, 588.0F), 18.0F, 18.0F), brush_);
    brush_->SetColor(WithOpacity(accent, entrance));
    target_->FillRectangle(D2D1::RectF(52.0F, 126.0F, 60.0F, 588.0F), brush_);
    drawText(L"PLAYER 1", detailFormat_, D2D1::RectF(82.0F, 160.0F, 244.0F, 202.0F), WithOpacity(heading, entrance));
    drawText(L"SINGLE", instructionFormat_, D2D1::RectF(82.0F, 224.0F, 244.0F, 254.0F), WithOpacity(detail, entrance));
    drawText(L"SPEED", instructionFormat_, D2D1::RectF(82.0F, 302.0F, 244.0F, 332.0F), WithOpacity(detail, entrance));
    drawText(L"x 1.0", detailFormat_, D2D1::RectF(82.0F, 334.0F, 244.0F, 372.0F), WithOpacity(heading, entrance));
    drawText(L"ENTER  PLAY", instructionFormat_, D2D1::RectF(82.0F, 454.0F, 244.0F, 484.0F), WithOpacity(detail, motion.instructionReveal));
    drawText(L"ESC  BACK", instructionFormat_, D2D1::RectF(82.0F, 496.0F, 244.0F, 526.0F), WithOpacity(detail, motion.instructionReveal));

    const auto centerOffset = (1.0F - entrance) * 82.0F;
    drawSongCard(D2D1::RectF(328.0F, 92.0F + centerOffset, 956.0F, 345.0F + centerOffset),
        L"SELECTED SONG", song.title, true);
    drawText(song.artist.empty() ? L"ARTIST UNKNOWN" : song.artist, detailFormat_,
        D2D1::RectF(370.0F, 282.0F + centerOffset, 914.0F, 327.0F + centerOffset), WithOpacity(detail, motion.detailReveal));

    drawSongCard(D2D1::RectF(292.0F, 398.0F, 512.0F, 498.0F), L"PREVIOUS",
        song.previousTitle.empty() ? L"—" : song.previousTitle, false);
    drawSongCard(D2D1::RectF(768.0F, 398.0F, 988.0F, 498.0F), L"NEXT",
        song.nextTitle.empty() ? L"—" : song.nextTitle, false);
    drawSongCard(D2D1::RectF(500.0F, 542.0F, 780.0F, 662.0F), L"CURRENT", song.title, true);

    brush_->SetColor(WithOpacity(panel, 0.94F * entrance));
    target_->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(500.0F, 382.0F, 780.0F, 516.0F), 14.0F, 14.0F), brush_);
    brush_->SetColor(WithOpacity(accent, song.difficultySelectionActive ? 0.80F + pulse * 0.20F : 0.38F));
    target_->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(500.0F, 382.0F, 780.0F, 516.0F), 14.0F, 14.0F), brush_, 3.0F);
    drawText(L"DIFFICULTY", instructionFormat_, D2D1::RectF(524.0F, 400.0F, 756.0F, 428.0F), WithOpacity(detail, entrance));
    drawText(song.difficultyName, detailFormat_, D2D1::RectF(524.0F, 432.0F, 690.0F, 478.0F), WithOpacity(heading, entrance));
    drawText(L"LV. " + std::to_wstring(song.difficultyLevel), detailFormat_,
        D2D1::RectF(684.0F, 432.0F, 756.0F, 478.0F), WithOpacity(accent, entrance));
    drawText(std::to_wstring(song.selectedDifficultyNumber) + L" / " + std::to_wstring(song.difficultyCount), instructionFormat_,
        D2D1::RectF(524.0F, 478.0F, 756.0F, 504.0F), WithOpacity(detail, entrance));

    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->EndDraw();
}

void SceneOverlayRenderer::DrawGameplay(
    const core::ViewportRect& viewport,
    const SceneOverlayText& text,
    const assets::ThemePalette& palette,
    const std::span<const GameplayRenderItem> items,
    const std::array<bool, 5>& pressedPanels,
    const GameplayHud& hud,
    const float energyPercent) {
    if (target_ == nullptr || brush_ == nullptr || viewport.scale <= 0.0F) {
        return;
    }

    target_->BeginDraw();
    target_->SetTransform(
        D2D1::Matrix3x2F::Scale(viewport.scale, viewport.scale)
        * D2D1::Matrix3x2F::Translation(viewport.x, viewport.y));

    if (gameplayVideoFrame_ != nullptr) {
        target_->DrawBitmap(gameplayVideoFrame_, D2D1::RectF(0.0F, 0.0F, 1280.0F, 720.0F), 0.54F);
    } else if (gameplayBackground_ != nullptr) {
        target_->DrawBitmap(gameplayBackground_, D2D1::RectF(0.0F, 0.0F, 1280.0F, 720.0F), 0.38F);
    } else {
        brush_->SetColor(D2D1::ColorF(0.025F, 0.06F, 0.12F, 1.0F));
        target_->FillRectangle(D2D1::RectF(0.0F, 0.0F, 1280.0F, 720.0F), brush_);
    }

    brush_->SetColor(D2D1::ColorF(0.01F, 0.02F, 0.04F, 0.70F));
    target_->FillRectangle(D2D1::RectF(0.0F, 0.0F, 1280.0F, 88.0F), brush_);

    brush_->SetColor(ToD2DColor(palette.heading));
    target_->DrawText(
        text.headline.data(), static_cast<UINT32>(text.headline.size()), detailFormat_,
        D2D1::RectF(760.0F, 18.0F, 1170.0F, 56.0F), brush_);
    brush_->SetColor(ToD2DColor(palette.detail));
    target_->DrawText(
        text.detail.data(), static_cast<UINT32>(text.detail.size()), instructionFormat_,
        D2D1::RectF(760.0F, 54.0F, 1170.0F, 82.0F), brush_);

    brush_->SetColor(D2D1::ColorF(0.005F, 0.01F, 0.025F, 0.72F));
    target_->FillRoundedRectangle(
        D2D1::RoundedRect(
            D2D1::RectF(
                layout::kFieldLeft - 22.0F,
                layout::kFieldTop - 16.0F,
                layout::kFieldRight + 22.0F,
                layout::kFieldBottom + 14.0F),
            18.0F,
            18.0F),
        brush_);

    const auto clampedEnergy = (std::clamp)(energyPercent, 0.0F, 100.0F);
    const auto gaugeFillRight = layout::kGaugeLeft
        + (layout::kGaugeRight - layout::kGaugeLeft) * clampedEnergy / 100.0F;
    const auto gaugeImpact = hud.feedback.gaugeImpact;
    if (std::abs(gaugeImpact) > 0.01F) {
        brush_->SetColor(WithOpacity(
            gaugeImpact > 0.0F ? ToD2DColor(palette.accent) : D2D1::ColorF(0.92F, 0.18F, 0.16F, 1.0F),
            0.12F + std::abs(gaugeImpact) * 0.28F));
        target_->FillRoundedRectangle(
            D2D1::RoundedRect(
                D2D1::RectF(
                    layout::kGaugeLeft - 6.0F,
                    layout::kGaugeTop - 6.0F,
                    layout::kGaugeRight + 6.0F,
                    layout::kGaugeBottom + 6.0F),
                13.0F,
                13.0F),
            brush_);
    }
    brush_->SetColor(D2D1::ColorF(0.01F, 0.02F, 0.04F, 0.84F));
    target_->FillRoundedRectangle(
        D2D1::RoundedRect(
            D2D1::RectF(layout::kGaugeLeft, layout::kGaugeTop, layout::kGaugeRight, layout::kGaugeBottom),
            10.0F,
            10.0F),
        brush_);
    if (clampedEnergy > 0.0F) {
        brush_->SetColor(clampedEnergy > 25.0F ? ToD2DColor(palette.accent) : D2D1::ColorF(0.92F, 0.18F, 0.16F, 1.0F));
        target_->FillRoundedRectangle(
            D2D1::RoundedRect(
                D2D1::RectF(
                    layout::kGaugeLeft + 5.0F,
                    layout::kGaugeTop + 5.0F,
                    (std::max)(layout::kGaugeLeft + 5.0F, gaugeFillRight - 5.0F),
                    layout::kGaugeBottom - 5.0F),
                7.0F,
                7.0F),
            brush_);
    }
    brush_->SetColor(ToD2DColor(palette.heading));
    target_->DrawRoundedRectangle(
        D2D1::RoundedRect(
            D2D1::RectF(layout::kGaugeLeft, layout::kGaugeTop, layout::kGaugeRight, layout::kGaugeBottom),
            10.0F,
            10.0F),
        brush_,
        2.0F);
    brush_->SetColor(ToD2DColor(palette.instruction));
    target_->DrawText(L"ENERGY", 6, instructionFormat_, D2D1::RectF(82.0F, 20.0F, 180.0F, 48.0F), brush_);

    const auto scoreText = L"SCORE  " + std::to_wstring(hud.score);
    const auto maxComboText = L"MAX " + std::to_wstring(hud.maxCombo) + L"   HOLD " + std::to_wstring(hud.holdTicks);
    brush_->SetColor(ToD2DColor(palette.heading));
    target_->DrawText(scoreText.data(), static_cast<UINT32>(scoreText.size()), detailFormat_,
        D2D1::RectF(948.0F, 18.0F, 1230.0F, 52.0F), brush_);
    brush_->SetColor(ToD2DColor(palette.instruction));
    target_->DrawText(maxComboText.data(), static_cast<UINT32>(maxComboText.size()), instructionFormat_,
        D2D1::RectF(948.0F, 54.0F, 1230.0F, 82.0F), brush_);

    for (std::uint8_t lane = 0; lane < 5; ++lane) {
        const auto left = layout::kFieldLeft + static_cast<float>(lane) * (layout::kLaneWidth + layout::kLaneGap);
        const auto centerX = left + layout::kLaneWidth * 0.5F;
        brush_->SetColor(ToD2DColor(palette.detail));
        target_->DrawRectangle(D2D1::RectF(left, layout::kFieldTop, left + layout::kLaneWidth, layout::kFieldBottom), brush_, 1.0F);

        const auto receptorImpact = hud.feedback.receptorImpact[lane];
        if (std::abs(receptorImpact) > 0.01F) {
            brush_->SetColor(WithOpacity(
                receptorImpact > 0.0F ? ToD2DColor(palette.accent) : D2D1::ColorF(0.92F, 0.18F, 0.16F, 1.0F),
                0.12F + std::abs(receptorImpact) * 0.34F));
            const auto radius = 42.0F + std::abs(receptorImpact) * 24.0F;
            target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(centerX, layout::kReceptorY), radius, radius), brush_);
        }
        if (pressedPanels[lane]) {
            brush_->SetColor(D2D1::ColorF(0.12F, 0.85F, 1.0F, 0.18F));
            target_->FillRectangle(D2D1::RectF(left, layout::kReceptorY - 48.0F, left + layout::kLaneWidth, layout::kReceptorY + 48.0F), brush_);
        }
        D2D1_MATRIX_3X2_F worldTransform{};
        target_->GetTransform(&worldTransform);
        DrawPanelGlyph(
            target_, brush_, panelArrowGeometry_, worldTransform, lane, centerX, layout::kReceptorY, 0.76F,
            pressedPanels[lane] ? ToD2DColor(palette.accent) : ToD2DColor(palette.panel),
            ToD2DColor(palette.heading), 2.0F);
    }

    target_->PushAxisAlignedClip(
        D2D1::RectF(layout::kFieldLeft, layout::kFieldTop, layout::kFieldRight, layout::kFieldBottom),
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    for (const auto& item : items) {
        if (item.lane >= 5) {
            continue;
        }
        const auto laneLeft = layout::kFieldLeft + static_cast<float>(item.lane) * (layout::kLaneWidth + layout::kLaneGap);
        const auto centerX = laneLeft + layout::kLaneWidth * 0.5F;
        if (item.isHold) {
            brush_->SetColor(item.isHoldActive
                ? ToD2DColor(palette.accent)
                : (item.isHoldDamaged ? D2D1::ColorF(0.92F, 0.18F, 0.16F, 1.0F) : ToD2DColor(palette.detail)));
            target_->FillRoundedRectangle(
                D2D1::RoundedRect(
                    D2D1::RectF(
                        centerX - 22.0F,
                        (std::min)(item.holdBodyStartY, item.tailY),
                        centerX + 22.0F,
                        (std::max)(item.holdBodyStartY, item.tailY)),
                    12.0F,
                    12.0F),
                brush_);
            if (item.isHoldDamaged) {
                brush_->SetColor(D2D1::ColorF(1.0F, 0.66F, 0.25F, 1.0F));
                target_->DrawRoundedRectangle(
                    D2D1::RoundedRect(
                        D2D1::RectF(
                            centerX - 22.0F,
                            (std::min)(item.holdBodyStartY, item.tailY),
                            centerX + 22.0F,
                            (std::max)(item.holdBodyStartY, item.tailY)),
                        12.0F,
                        12.0F),
                    brush_,
                    2.0F);
            }
        }

        if (item.showHead) {
            D2D1_MATRIX_3X2_F worldTransform{};
            target_->GetTransform(&worldTransform);
            DrawPanelGlyph(
                target_, brush_, panelArrowGeometry_, worldTransform, item.lane, centerX, item.headY, 0.70F,
                item.isHold && item.isHoldActive ? ToD2DColor(palette.heading) : ToD2DColor(palette.accent),
                ToD2DColor(palette.heading), 2.0F);
        }
    }
    target_->PopAxisAlignedClip();

    headlineFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    const auto judgementBurst = hud.feedback.judgementBurst;
    if (judgementBurst > 0.01F) {
        brush_->SetColor(WithOpacity(ToD2DColor(palette.accent), judgementBurst * 0.34F));
        const auto radius = 44.0F + judgementBurst * 42.0F;
        target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(448.0F, 240.0F), radius * 1.55F, radius * 0.65F), brush_, 2.0F);
    }
    D2D1_MATRIX_3X2_F hudTransform{};
    target_->GetTransform(&hudTransform);
    const auto judgementScale = 1.0F + judgementBurst * 0.16F;
    target_->SetTransform(hudTransform * D2D1::Matrix3x2F::Scale(
        judgementScale, judgementScale, D2D1::Point2F(448.0F, 240.0F)));
    brush_->SetColor(WithOpacity(ToD2DColor(palette.heading), 0.66F + judgementBurst * 0.34F));
    target_->DrawText(hud.judgement.data(), static_cast<UINT32>(hud.judgement.size()), headlineFormat_,
        D2D1::RectF(88.0F, 206.0F, 808.0F, 274.0F), brush_);
    target_->SetTransform(hudTransform);
    brush_->SetColor(ToD2DColor(palette.instruction));
    target_->DrawText(L"COMBO", 5, instructionFormat_, D2D1::RectF(258.0F, 278.0F, 638.0F, 304.0F), brush_);
    const auto comboText = std::to_wstring(hud.combo);
    target_->SetTransform(hudTransform * D2D1::Matrix3x2F::Scale(
        hud.feedback.comboScale, hud.feedback.comboScale, D2D1::Point2F(448.0F, 334.0F)));
    brush_->SetColor(ToD2DColor(palette.accent));
    target_->DrawText(comboText.data(), static_cast<UINT32>(comboText.size()), headlineFormat_,
        D2D1::RectF(158.0F, 300.0F, 738.0F, 370.0F), brush_);
    target_->SetTransform(hudTransform);
    headlineFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

    brush_->SetColor(ToD2DColor(palette.instruction));
    target_->DrawText(
        text.instruction.data(), static_cast<UINT32>(text.instruction.size()), instructionFormat_,
        D2D1::RectF(128.0F, 686.0F, 768.0F, 716.0F), brush_);

    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->EndDraw();
}

} // namespace pumpdx::render
