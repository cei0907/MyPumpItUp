#pragma once

#include "framework/assets/ResourceCache.hpp"
#include "framework/render/LogicalViewport.hpp"
#include "framework/render/SceneOverlayRenderer.hpp"
#include "framework/video/MediaFoundationBgaPlayer.hpp"
#include "game/application/GameFlow.hpp"

#include <Windows.h>
#include <d3d11.h>

#include <cstdint>

namespace pumpdx::game {

class GameApplication final {
public:
    explicit GameApplication(HINSTANCE instanceHandle);
    ~GameApplication();

    GameApplication(const GameApplication&) = delete;
    GameApplication& operator=(const GameApplication&) = delete;

    [[nodiscard]] int Run();

private:
    static constexpr wchar_t kWindowClassName[] = L"PumpDXRebuildWindow";
    static constexpr wchar_t kWindowTitle[] = L"PumpDX Rebuild";

    static LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    [[nodiscard]] bool CreateMainWindow();
    [[nodiscard]] bool CreateGraphicsDevice();
    [[nodiscard]] bool CreateRenderTarget();
    void ReleaseRenderTarget();
    void ReleaseGraphicsDevice();
    void Resize(std::uint32_t width, std::uint32_t height);
    void HandleKeyPressed(std::uint32_t virtualKey);
    void HandleKeyReleased(std::uint32_t virtualKey);
    void RenderFrame();

    HINSTANCE instanceHandle_ = nullptr;
    HWND window_ = nullptr;
    bool isMinimized_ = false;

    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    ID3D11RenderTargetView* renderTargetView_ = nullptr;

    core::ViewportRect logicalViewport_{};
    assets::ResourceCache resourceCache_{};
    render::SceneOverlayRenderer sceneOverlayRenderer_{};
    video::MediaFoundationBgaPlayer bgaVideoPlayer_{};
    GameFlow gameFlow_{};
};

} // namespace pumpdx::game
