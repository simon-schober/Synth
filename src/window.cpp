#include "window.h"
#include "imgui_impl_win32.h"
#include "helpers.h"
#include <dxgi1_5.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool CreateApplicationWindow(WindowContext& context, int width, int height)
{
    ImGui_ImplWin32_EnableDpiAwareness();
    context.dpi_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    context.wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), 
                   nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    
    if (!::RegisterClassExW(&context.wc))
        return false;

    context.hwnd = ::CreateWindowW(
        context.wc.lpszClassName, 
        L"Dear ImGui DirectX12 Example", 
        WS_OVERLAPPEDWINDOW,
        100, 100, 
        static_cast<int>(width * context.dpi_scale), 
        static_cast<int>(height * context.dpi_scale),
        nullptr, nullptr, context.wc.hInstance, nullptr
    );

    if (!context.hwnd)
        return false;

    ::ShowWindow(context.hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(context.hwnd);

    return true;
}

void DestroyApplicationWindow(const WindowContext& context)
{
    ::DestroyWindow(context.hwnd);
    ::UnregisterClassW(context.wc.lpszClassName, context.wc.hInstance);
}

bool ProcessWindowMessages(bool& done)
{
    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
        {
            done = true;
            return false;
        }
    }
    return true;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            DXGI_SWAP_CHAIN_DESC1 desc = {};
            g_pSwapChain->GetDesc1(&desc);
            HRESULT result = g_pSwapChain->ResizeBuffers(0, static_cast<UINT>(LOWORD(lParam)), 
                                                          static_cast<UINT>(HIWORD(lParam)), 
                                                          desc.Format, desc.Flags);
            IM_ASSERT(SUCCEEDED(result) && "Failed to resize swapchain.");
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}