#pragma once

#include "framework/render/LogicalViewport.hpp"
#include "game/scenes/SceneManager.hpp"

#include <Windows.h>
#include <d3d11.h>

#include <cstdint>

namespace pumpdx::app {

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
    scenes::SceneManager sceneManager_{};
};

} // namespace pumpdx::app
