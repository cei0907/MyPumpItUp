#include "framework/render/SceneOverlayRenderer.hpp"

#include <d2d1helper.h>
#include <dxgi.h>

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

void SceneOverlayRenderer::ReleaseTarget() {
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
    const std::array<bool, 5>& pressedPanels) {
    if (target_ == nullptr || brush_ == nullptr || viewport.scale <= 0.0F) {
        return;
    }

    constexpr float fieldLeft = 190.0F;
    constexpr float laneWidth = 150.0F;
    constexpr float laneGap = 12.0F;
    constexpr float fieldTop = 95.0F;
    constexpr float fieldBottom = 675.0F;
    constexpr float receptorY = 525.0F;

    target_->BeginDraw();
    target_->SetTransform(
        D2D1::Matrix3x2F::Scale(viewport.scale, viewport.scale)
        * D2D1::Matrix3x2F::Translation(viewport.x, viewport.y));

    brush_->SetColor(ToD2DColor(palette.heading));
    target_->DrawText(
        text.headline.data(), static_cast<UINT32>(text.headline.size()), detailFormat_,
        D2D1::RectF(44.0F, 20.0F, 420.0F, 62.0F), brush_);
    brush_->SetColor(ToD2DColor(palette.detail));
    target_->DrawText(
        text.detail.data(), static_cast<UINT32>(text.detail.size()), instructionFormat_,
        D2D1::RectF(360.0F, 24.0F, 1120.0F, 58.0F), brush_);

    brush_->SetColor(ToD2DColor(palette.panel));
    target_->FillRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(fieldLeft - 18.0F, fieldTop - 14.0F, 1090.0F, fieldBottom + 14.0F), 18.0F, 18.0F),
        brush_);

    for (std::uint8_t lane = 0; lane < 5; ++lane) {
        const auto left = fieldLeft + static_cast<float>(lane) * (laneWidth + laneGap);
        brush_->SetColor(ToD2DColor(palette.detail));
        target_->DrawRectangle(D2D1::RectF(left, fieldTop, left + laneWidth, fieldBottom), brush_, 1.0F);

        brush_->SetColor(pressedPanels[lane] ? ToD2DColor(palette.accent) : ToD2DColor(palette.panel));
        target_->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(left + 18.0F, receptorY - 24.0F, left + laneWidth - 18.0F, receptorY + 24.0F), 9.0F, 9.0F),
            brush_);
        brush_->SetColor(ToD2DColor(palette.heading));
        target_->DrawRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(left + 18.0F, receptorY - 24.0F, left + laneWidth - 18.0F, receptorY + 24.0F), 9.0F, 9.0F),
            brush_, 2.0F);
    }

    target_->PushAxisAlignedClip(D2D1::RectF(fieldLeft, fieldTop, 1090.0F, fieldBottom), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    for (const auto& item : items) {
        if (item.lane >= 5) {
            continue;
        }
        const auto laneLeft = fieldLeft + static_cast<float>(item.lane) * (laneWidth + laneGap);
        const auto centerX = laneLeft + laneWidth * 0.5F;
        if (item.isHold) {
            brush_->SetColor(ToD2DColor(palette.detail));
            target_->FillRoundedRectangle(
                D2D1::RoundedRect(
                    D2D1::RectF(
                        centerX - 22.0F,
                        (std::min)(item.headY, item.tailY),
                        centerX + 22.0F,
                        (std::max)(item.headY, item.tailY)),
                    12.0F,
                    12.0F),
                brush_);
        }

        brush_->SetColor(ToD2DColor(palette.accent));
        target_->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(centerX - 45.0F, item.headY - 18.0F, centerX + 45.0F, item.headY + 18.0F), 8.0F, 8.0F),
            brush_);
        brush_->SetColor(ToD2DColor(palette.heading));
        target_->DrawRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(centerX - 45.0F, item.headY - 18.0F, centerX + 45.0F, item.headY + 18.0F), 8.0F, 8.0F),
            brush_, 2.0F);
    }
    target_->PopAxisAlignedClip();

    brush_->SetColor(ToD2DColor(palette.instruction));
    target_->DrawText(
        text.instruction.data(), static_cast<UINT32>(text.instruction.size()), instructionFormat_,
        D2D1::RectF(300.0F, 688.0F, 1120.0F, 718.0F), brush_);

    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->EndDraw();
}

} // namespace pumpdx::render
