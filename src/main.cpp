// Dear ImGui: standalone example application for Windows API + DirectX 12

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "helpers.h"
#include "audio.h"
#include "imgui_setup.h"
#include "renderer.h"

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

// Global variable definitions
AudioState g_audioState;
FrameContext g_frameContext[APP_NUM_FRAMES_IN_FLIGHT] = {};
UINT g_frameIndex = 0;

ID3D12Device* g_pd3dDevice = nullptr;
ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;
ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
ID3D12Fence* g_fence = nullptr;
HANDLE g_fenceEvent = nullptr;
UINT64 g_fenceLastSignaledValue = 0;
IDXGISwapChain3* g_pSwapChain = nullptr;
bool g_SwapChainTearingSupport = false;
bool g_SwapChainOccluded = false;
HANDLE g_hSwapChainWaitableObject = nullptr;
ID3D12Resource* g_mainRenderTargetResource[APP_NUM_BACK_BUFFERS] = {};
D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[APP_NUM_BACK_BUFFERS] = {};

// Forward declarations
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void EditContent(bool& show_demo_window, bool& show_another_window, float& f, ImVec4& clear_color, int& counter, ImGuiIO& io);
bool ProcessMessages(bool& done);

int main(int, char**)
{
    // Initialize DPI and create window
    ImGui_ImplWin32_EnableDpiAwareness();
    float dpi_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX12 Example", WS_OVERLAPPEDWINDOW, 
                                 100, 100, (int)(1280 * dpi_scale), (int)(800 * dpi_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize D3D12
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup ImGui
    SetupImGui(hwnd, dpi_scale);

    // Initialize audio
    ma_device device;
    if (!InitializeAudio(device))
        return -1;

    // Application state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    float frequency = 440.0f;
    int counter = 0;

    // Main loop
    bool done = false;
    while (!done)
    {
        if (!ProcessMessages(done))
            break;

        // Handle minimized/occluded window
        if ((g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) || ::IsIconic(hwnd))
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        EditContent(show_demo_window, show_another_window, frequency, clear_color, counter, ImGui::GetIO());

        ImGui::Render();
        RenderFrame(clear_color);
    }

    WaitForPendingOperations();

    // Cleanup
    CleanupAudio(device);
    ShutdownImGui();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

void EditContent(bool& show_demo_window, bool& show_another_window, float& f, ImVec4& clear_color, int& counter, ImGuiIO& io)
{
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    {
        ImGui::Begin("Hello, world!");
        ImGui::Text("This is some useful text.");
        ImGui::Checkbox("Demo Window", &show_demo_window);
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("Frequency (Hz)", &f, 20.0f, 2000.0f);
        g_audioState.frequency = f;

        ImGui::ColorEdit3("clear color", (float*)&clear_color);

        if (ImGui::Button("Button"))
            counter++;
        if (ImGui::Button("Reset Counter"))
            counter = 0;

        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}

bool ProcessMessages(bool& done)
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

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
            HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), desc.Format, desc.Flags);
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