#include "framework/render/SceneOverlayRenderer.hpp"

#include "framework/render/GameplayLayout.hpp"

#include <d2d1helper.h>
#include <dxgi.h>
#include <objbase.h>
#include <wincodec.h>

#include <algorithm>
#include <string>

namespace pumpdx::render {

namespace {

[[nodiscard]] D2D1_COLOR_F ToD2DColor(const std::array<float, 4>& color) {
    return {color[0], color[1], color[2], color[3]};
}

template <typename T>
void ReleaseCom(T*& object) {
    if (object != nullptr) {
        object->Release();
        object = nullptr;
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

void SceneOverlayRenderer::ReleaseTarget() {
    ReleaseCom(gameplayBackground_);
    backgroundLoadAttempted_ = false;
    ReleaseCom(brush_);
    ReleaseCom(target_);
}

void SceneOverlayRenderer::Shutdown() {
    ReleaseTarget();
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
    const assets::ThemePalette& palette) {
    if (target_ == nullptr || brush_ == nullptr || viewport.scale <= 0.0F) {
        return;
    }

    target_->BeginDraw();
    target_->SetTransform(
        D2D1::Matrix3x2F::Scale(viewport.scale, viewport.scale)
        * D2D1::Matrix3x2F::Translation(viewport.x, viewport.y));

    brush_->SetColor(ToD2DColor(palette.panel));
    target_->FillRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(106.0F, 112.0F, 1174.0F, 608.0F), 28.0F, 28.0F), brush_);

    brush_->SetColor(ToD2DColor(palette.accent));
    target_->FillRectangle(D2D1::RectF(156.0F, 197.0F, 166.0F, 438.0F), brush_);

    brush_->SetColor(ToD2DColor(palette.heading));
    target_->DrawText(
        text.headline.data(), static_cast<UINT32>(text.headline.size()), headlineFormat_,
        D2D1::RectF(204.0F, 170.0F, 1100.0F, 260.0F), brush_);

    brush_->SetColor(ToD2DColor(palette.detail));
    target_->DrawText(
        text.detail.data(), static_cast<UINT32>(text.detail.size()), detailFormat_,
        D2D1::RectF(204.0F, 294.0F, 1080.0F, 360.0F), brush_);

    brush_->SetColor(ToD2DColor(palette.instruction));
    target_->DrawText(
        text.instruction.data(), static_cast<UINT32>(text.instruction.size()), instructionFormat_,
        D2D1::RectF(204.0F, 468.0F, 1080.0F, 526.0F), brush_);

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

    if (gameplayBackground_ != nullptr) {
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
        D2D1::RectF(44.0F, 18.0F, 600.0F, 56.0F), brush_);
    brush_->SetColor(ToD2DColor(palette.detail));
    target_->DrawText(
        text.detail.data(), static_cast<UINT32>(text.detail.size()), instructionFormat_,
        D2D1::RectF(44.0F, 54.0F, 600.0F, 82.0F), brush_);

    brush_->SetColor(ToD2DColor(palette.panel));
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
    target_->DrawText(L"ENERGY", 6, instructionFormat_, D2D1::RectF(218.0F, 108.0F, 324.0F, 136.0F), brush_);

    const auto scoreText = L"SCORE  " + std::to_wstring(hud.score);
    const auto maxComboText = L"MAX " + std::to_wstring(hud.maxCombo) + L"   HOLD " + std::to_wstring(hud.holdTicks);
    brush_->SetColor(ToD2DColor(palette.heading));
    target_->DrawText(scoreText.data(), static_cast<UINT32>(scoreText.size()), detailFormat_,
        D2D1::RectF(928.0F, 18.0F, 1230.0F, 52.0F), brush_);
    brush_->SetColor(ToD2DColor(palette.instruction));
    target_->DrawText(maxComboText.data(), static_cast<UINT32>(maxComboText.size()), instructionFormat_,
        D2D1::RectF(928.0F, 54.0F, 1230.0F, 82.0F), brush_);

    for (std::uint8_t lane = 0; lane < 5; ++lane) {
        const auto left = layout::kFieldLeft + static_cast<float>(lane) * (layout::kLaneWidth + layout::kLaneGap);
        brush_->SetColor(ToD2DColor(palette.detail));
        target_->DrawRectangle(D2D1::RectF(left, layout::kFieldTop, left + layout::kLaneWidth, layout::kFieldBottom), brush_, 1.0F);

        if (pressedPanels[lane]) {
            brush_->SetColor(D2D1::ColorF(0.12F, 0.85F, 1.0F, 0.18F));
            target_->FillRectangle(D2D1::RectF(left, layout::kReceptorY - 48.0F, left + layout::kLaneWidth, layout::kReceptorY + 48.0F), brush_);
        }
        brush_->SetColor(pressedPanels[lane] ? ToD2DColor(palette.accent) : ToD2DColor(palette.panel));
        target_->FillRoundedRectangle(
            D2D1::RoundedRect(
                D2D1::RectF(left + 14.0F, layout::kReceptorY - 25.0F, left + layout::kLaneWidth - 14.0F, layout::kReceptorY + 25.0F),
                9.0F,
                9.0F),
            brush_);
        brush_->SetColor(ToD2DColor(palette.heading));
        target_->DrawRoundedRectangle(
            D2D1::RoundedRect(
                D2D1::RectF(left + 14.0F, layout::kReceptorY - 25.0F, left + layout::kLaneWidth - 14.0F, layout::kReceptorY + 25.0F),
                9.0F,
                9.0F),
            brush_, 2.0F);
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
            brush_->SetColor(item.isHold && item.isHoldActive ? ToD2DColor(palette.heading) : ToD2DColor(palette.accent));
            target_->FillRoundedRectangle(
                D2D1::RoundedRect(D2D1::RectF(centerX - 45.0F, item.headY - 18.0F, centerX + 45.0F, item.headY + 18.0F), 8.0F, 8.0F),
                brush_);
            brush_->SetColor(ToD2DColor(palette.heading));
            target_->DrawRoundedRectangle(
                D2D1::RoundedRect(D2D1::RectF(centerX - 45.0F, item.headY - 18.0F, centerX + 45.0F, item.headY + 18.0F), 8.0F, 8.0F),
                brush_, 2.0F);
        }
    }
    target_->PopAxisAlignedClip();

    headlineFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    brush_->SetColor(ToD2DColor(palette.heading));
    target_->DrawText(hud.judgement.data(), static_cast<UINT32>(hud.judgement.size()), headlineFormat_,
        D2D1::RectF(390.0F, 178.0F, 890.0F, 246.0F), brush_);
    brush_->SetColor(ToD2DColor(palette.instruction));
    target_->DrawText(L"COMBO", 5, instructionFormat_, D2D1::RectF(548.0F, 250.0F, 732.0F, 276.0F), brush_);
    const auto comboText = std::to_wstring(hud.combo);
    brush_->SetColor(ToD2DColor(palette.accent));
    target_->DrawText(comboText.data(), static_cast<UINT32>(comboText.size()), headlineFormat_,
        D2D1::RectF(460.0F, 272.0F, 820.0F, 342.0F), brush_);
    headlineFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

    brush_->SetColor(ToD2DColor(palette.instruction));
    target_->DrawText(
        text.instruction.data(), static_cast<UINT32>(text.instruction.size()), instructionFormat_,
        D2D1::RectF(326.0F, 686.0F, 954.0F, 716.0F), brush_);

    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->EndDraw();
}

} // namespace pumpdx::render
