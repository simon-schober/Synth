// Dear ImGui: standalone example application for Windows API + DirectX 12

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "graphics_setup.h"
#include "audio.h"
#include "imgui_setup.h"
#include "renderer.h"
#include "window.h"
#include "midi.h"
#include "synth_interface.h"

#include <cmath>
#include <cstdio>

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

int main(int, char**)
{
    // Create window
    WindowContext window_context;
    if (!CreateApplicationWindow(window_context))
        return 1;

    // Initialize D3D12
    if (!CreateDeviceD3D(window_context.hwnd))
    {
        CleanupDeviceD3D();
        DestroyApplicationWindow(window_context);
        return 1;
    }

    // Setup ImGui
    if (!SetupImGui(window_context.hwnd, window_context.dpi_scale))
    {
        CleanupDeviceD3D();
        DestroyApplicationWindow(window_context);
        return 1;
    }

    // Initialize audio
    ma_device device;
    if (!InitializeAudio(device))
    {
        ShutdownImGui();
        CleanupDeviceD3D();
        DestroyApplicationWindow(window_context);
        return 1;
    }

    // Initialize MIDI
    MidiContext midi_context;

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
        if (!ProcessWindowMessages(done))
            break;

        // Handle minimized/occluded window
        if ((g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) || 
            ::IsIconic(window_context.hwnd))
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        CreateNoteInputInterface(show_demo_window, show_another_window, frequency, clear_color, counter, ImGui::GetIO());

        ImGui::Render();
        RenderFrame(clear_color);
    }

    WaitForPendingOperations();

    // Cleanup
    CleanupMidi(midi_context);
    CleanupAudio(device);
    ShutdownImGui();
    CleanupDeviceD3D();
    DestroyApplicationWindow(window_context);

    return 0;
}
