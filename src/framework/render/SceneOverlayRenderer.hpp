#pragma once

#include "framework/render/LogicalViewport.hpp"

#include <d2d1.h>
#include <d3d11.h>
#include <dwrite.h>

#include <string_view>

namespace pumpdx::render {

struct SceneOverlayText final {
    std::wstring_view headline;
    std::wstring_view detail;
    std::wstring_view instruction;
};

class SceneOverlayRenderer final {
public:
    SceneOverlayRenderer() = default;
    ~SceneOverlayRenderer();

    SceneOverlayRenderer(const SceneOverlayRenderer&) = delete;
    SceneOverlayRenderer& operator=(const SceneOverlayRenderer&) = delete;

    [[nodiscard]] bool Initialize();
    [[nodiscard]] bool CreateTarget(ID3D11Texture2D* backBuffer);
    void ReleaseTarget();
    void Shutdown();
    void Draw(const core::ViewportRect& viewport, const SceneOverlayText& text);

private:
    ID2D1Factory* factory_ = nullptr;
    ID2D1RenderTarget* target_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;
    IDWriteFactory* writeFactory_ = nullptr;
    IDWriteTextFormat* headlineFormat_ = nullptr;
    IDWriteTextFormat* detailFormat_ = nullptr;
    IDWriteTextFormat* instructionFormat_ = nullptr;
};

} // namespace pumpdx::render
