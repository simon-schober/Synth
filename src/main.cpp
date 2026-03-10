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
#include "window.h"
#include "midi.h"

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

// Forward declaration
void CreateNoteInput(bool& show_demo_window, bool& show_another_window, float& f, ImVec4& clear_color, int& counter, ImGuiIO& io);

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
    InitializeMidi(midi_context, [](int note, int velocity, bool note_on) {
        // Note names for printing
        const char* note_names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        
        if (note_on)
        {
            // Calculate octave and note name
            int octave = (note / 12) - 1;
            int note_index = note % 12;
            
            // Convert MIDI note to frequency: f = 440 * 2^((note - 69) / 12)
            float freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
            g_audioState.frequency = freq;
            
            // Print note information
            printf("MIDI Note ON:  %s%d (MIDI#%d) - %.2f Hz - Velocity: %d\n", 
                   note_names[note_index], octave, note, freq, velocity);
        }
        else
        {
            // Calculate octave and note name for note off
            int octave = (note / 12) - 1;
            int note_index = note % 12;
            
            printf("MIDI Note OFF: %s%d (MIDI#%d)\n", 
                   note_names[note_index], octave, note);
        }
    });

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

        CreateNoteInput(show_demo_window, show_another_window, frequency, clear_color, counter, ImGui::GetIO());

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

void CreateNoteInput(bool& show_demo_window, bool& show_another_window, float& f, ImVec4& clear_color, int& counter, ImGuiIO& io)
{
    ImGui::Begin("12-TET Synthesizer");

    // Note names for chromatic scale
    const char* note_names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    
    // Calculate frequency for a given note
    // A4 (440 Hz) is note index 9 in octave 4
    // Formula: f = 440 * 2^((n - 69) / 12) where n is MIDI note number
    auto get_frequency = [](int octave, int note_index) -> float {
        int midi_note = (octave + 1) * 12 + note_index; // MIDI note number (C-1 = 0, A4 = 69)
        return 440.0f * std::pow(2.0f, (midi_note - 69) / 12.0f);
    };

    // Display current frequency
    ImGui::Text("Current Frequency: %.2f Hz", g_audioState.frequency);
    ImGui::Separator();

    // Create piano keyboard across 3 octaves (C3 to B5)
    for (int octave = 3; octave <= 5; ++octave)
    {
        ImGui::Text("Octave %d", octave);
        
        for (int note = 0; note < 12; ++note)
        {
            float freq = get_frequency(octave, note);
            
            // Color black keys differently
            bool is_black_key = (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);
            if (is_black_key)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 1.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 1.0f, 1.0f));
            }

            char button_label[32];
            snprintf(button_label, sizeof(button_label), "%s%d##%d%d", note_names[note], octave, octave, note);
            
            if (ImGui::Button(button_label, ImVec2(60, 40)))
            {
                g_audioState.frequency = freq;
                f = freq; // Update slider
            }

            ImGui::PopStyleColor(3);

            // Layout: show 6 notes per row
            if (note % 6 != 5)
                ImGui::SameLine();
        }
        
        ImGui::Spacing();
    }

    ImGui::Separator();
    
    // Manual frequency slider
    ImGui::Text("Manual Control:");
    if (ImGui::SliderFloat("Frequency (Hz)", &f, 20.0f, 2000.0f))
    {
        g_audioState.frequency = f;
    }

    ImGui::End();
}