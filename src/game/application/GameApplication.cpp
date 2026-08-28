#include "game/application/GameApplication.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <stdexcept>

namespace pumpdx::game {

GameApplication::GameApplication(const HINSTANCE instanceHandle)
    : instanceHandle_(instanceHandle) {
}

GameApplication::~GameApplication() {
    ReleaseGraphicsDevice();
}

int GameApplication::Run() {
    if (!CreateMainWindow()) {
        return 1;
    }

    if (!CreateGraphicsDevice()) {
        MessageBoxW(window_, L"Direct3D 11 장치를 만들 수 없습니다.", kWindowTitle, MB_ICONERROR | MB_OK);
        return 1;
    }

    ShowWindow(window_, SW_SHOWDEFAULT);
    UpdateWindow(window_);
    SetWindowTextW(window_, gameFlow_.CurrentSceneVisual().windowTitle.data());

    MSG message{};
    while (message.message != WM_QUIT) {
        if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != FALSE) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
            continue;
        }

        if (gameFlow_.Update()) {
            SetWindowTextW(window_, gameFlow_.CurrentSceneVisual().windowTitle.data());
        }
        RenderFrame();
    }

    return static_cast<int>(message.wParam);
}

bool GameApplication::CreateMainWindow() {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = instanceHandle_;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.lpszClassName = kWindowClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);

    if (RegisterClassExW(&windowClass) == 0) {
        return false;
    }

    RECT windowRect{
        .left = 0,
        .top = 0,
        .right = static_cast<LONG>(core::LogicalViewport::kDesignWidth),
        .bottom = static_cast<LONG>(core::LogicalViewport::kDesignHeight),
    };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    window_ = CreateWindowExW(
        0,
        kWindowClassName,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instanceHandle_,
        this);

    return window_ != nullptr;
}

bool GameApplication::CreateGraphicsDevice() {
    DXGI_SWAP_CHAIN_DESC swapChainDescription{};
    swapChainDescription.BufferCount = 1;
    swapChainDescription.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescription.OutputWindow = window_;
    swapChainDescription.SampleDesc.Count = 1;
    swapChainDescription.Windowed = TRUE;
    swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr std::array featureLevels{
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    auto result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        featureLevels.data(),
        static_cast<UINT>(featureLevels.size()),
        D3D11_SDK_VERSION,
        &swapChainDescription,
        &swapChain_,
        &device_,
        nullptr,
        &context_);

    if (FAILED(result)) {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            featureLevels.data(),
            static_cast<UINT>(featureLevels.size()),
            D3D11_SDK_VERSION,
            &swapChainDescription,
            &swapChain_,
            &device_,
            nullptr,
            &context_);
    }

    if (FAILED(result)) {
        return false;
    }

    try {
        resourceCache_.LoadTheme(std::filesystem::path(PUMP_DX_DEFAULT_THEME_MANIFEST));
    } catch (const std::exception&) {
        ReleaseGraphicsDevice();
        return false;
    }

    if (!sceneOverlayRenderer_.Initialize()) {
        ReleaseGraphicsDevice();
        return false;
    }

    RECT clientRect{};
    GetClientRect(window_, &clientRect);
    Resize(
        static_cast<std::uint32_t>(clientRect.right - clientRect.left),
        static_cast<std::uint32_t>(clientRect.bottom - clientRect.top));
    return renderTargetView_ != nullptr;
}

bool GameApplication::CreateRenderTarget() {
    ID3D11Texture2D* backBuffer = nullptr;
    const auto getBufferResult = swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(getBufferResult)) {
        return false;
    }

    const auto createViewResult = device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
    const auto createOverlayResult = SUCCEEDED(createViewResult) && sceneOverlayRenderer_.CreateTarget(backBuffer);
    backBuffer->Release();
    if (!createOverlayResult) {
        ReleaseRenderTarget();
        return false;
    }

    return true;
}

void GameApplication::ReleaseRenderTarget() {
    sceneOverlayRenderer_.ReleaseTarget();
    if (renderTargetView_ != nullptr) {
        renderTargetView_->Release();
        renderTargetView_ = nullptr;
    }
}

void GameApplication::ReleaseGraphicsDevice() {
    ReleaseRenderTarget();
    sceneOverlayRenderer_.Shutdown();

    if (swapChain_ != nullptr) {
        swapChain_->Release();
        swapChain_ = nullptr;
    }
    if (context_ != nullptr) {
        context_->Release();
        context_ = nullptr;
    }
    if (device_ != nullptr) {
        device_->Release();
        device_ = nullptr;
    }
}

void GameApplication::Resize(const std::uint32_t width, const std::uint32_t height) {
    if (width == 0 || height == 0 || swapChain_ == nullptr) {
        isMinimized_ = true;
        return;
    }

    isMinimized_ = false;
    logicalViewport_ = core::LogicalViewport::FitInside(width, height);

    ReleaseRenderTarget();
    if (FAILED(swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0))) {
        return;
    }

    if (!CreateRenderTarget()) {
        isMinimized_ = true;
    }
}

void GameApplication::HandleKeyReleased(const std::uint32_t virtualKey) {
    gameFlow_.HandleKeyReleased(virtualKey);
}

void GameApplication::HandleKeyPressed(const std::uint32_t virtualKey) {
    gameFlow_.HandleKeyPressed(virtualKey);
}

void GameApplication::RenderFrame() {
    if (isMinimized_ || renderTargetView_ == nullptr) {
        return;
    }

    const auto clearColor = gameFlow_.CurrentSceneVisual().clearColor;
    context_->OMSetRenderTargets(1, &renderTargetView_, nullptr);
    context_->ClearRenderTargetView(renderTargetView_, clearColor.data());

    const D3D11_VIEWPORT viewport{
        .TopLeftX = logicalViewport_.x,
        .TopLeftY = logicalViewport_.y,
        .Width = logicalViewport_.width,
        .Height = logicalViewport_.height,
        .MinDepth = 0.0F,
        .MaxDepth = 1.0F,
    };
    context_->RSSetViewports(1, &viewport);

    context_->OMSetRenderTargets(0, nullptr, nullptr);
    context_->Flush();

    const auto visual = gameFlow_.CurrentSceneVisual();
    const auto overlayText = render::SceneOverlayText{
        .headline = visual.headline,
        .detail = visual.detail,
        .instruction = visual.instruction,
    };
    const auto& palette = resourceCache_.ActiveTheme().Palette();
    if (const auto* gameplay = gameFlow_.ActiveGameplay(); gameplay != nullptr) {
        const auto videoOpened = bgaVideoPlayer_.Open(gameFlow_.ActiveVideoBgaPath());
        const auto* videoFrame = videoOpened ? bgaVideoPlayer_.FrameAt(gameplay->SongTimeSeconds()) : nullptr;
        if (videoFrame != nullptr && sceneOverlayRenderer_.LoadGameplayVideoFrame(*videoFrame)) {
            // The audio clock owns both note projection and video-frame selection.
        } else {
            sceneOverlayRenderer_.ClearGameplayVideoFrame();
            static_cast<void>(sceneOverlayRenderer_.LoadGameplayBackground(gameFlow_.ActiveStaticBgaPath()));
        }
        const auto items = gameplay->BuildRenderItemsForCurrentTime();
        const auto& score = gameplay->Score();
        const auto judgement = score.LatestJudgement().has_value()
            ? gameplay::JudgementLabel(*score.LatestJudgement())
            : std::wstring_view(L"READY");
        sceneOverlayRenderer_.DrawGameplay(
            logicalViewport_, overlayText, palette, items, gameplay->PressedPanels(),
            {
                .judgement = judgement,
                .score = score.Score(),
                .combo = score.CurrentCombo(),
                .maxCombo = score.MaxCombo(),
                .holdTicks = score.HoldTicks(),
                .feedback = gameplay->BuildFeedbackForCurrentTime(),
            },
            static_cast<float>(gameplay->Energy().Value()));
    } else {
        bgaVideoPlayer_.Close();
        sceneOverlayRenderer_.ClearGameplayVideoFrame();
        sceneOverlayRenderer_.Draw(logicalViewport_, overlayText, palette);
    }

    swapChain_->Present(1, 0);
}

LRESULT CALLBACK GameApplication::WindowProcedure(
    const HWND window,
    const UINT message,
    const WPARAM wParam,
    const LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
    }

    auto* application = reinterpret_cast<GameApplication*>(GetWindowLongPtrW(window, GWLP_USERDATA));

    switch (message) {
    case WM_SIZE:
        if (application != nullptr) {
            application->Resize(
                static_cast<std::uint32_t>(LOWORD(lParam)),
                static_cast<std::uint32_t>(HIWORD(lParam)));
        }
        return 0;
    case WM_KEYDOWN:
        if (application != nullptr) {
            application->HandleKeyPressed(static_cast<std::uint32_t>(wParam));
        }
        return 0;
    case WM_KEYUP:
        if (application != nullptr) {
            application->HandleKeyReleased(static_cast<std::uint32_t>(wParam));
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

} // namespace pumpdx::game
