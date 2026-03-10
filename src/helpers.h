#pragma once

#include <d3d12.h>
#include <dxgi1_5.h>
#include "imgui.h"

// Config constants - define once in header
inline constexpr int APP_NUM_FRAMES_IN_FLIGHT = 2;
inline constexpr int APP_NUM_BACK_BUFFERS = 2;
inline constexpr int APP_SRV_HEAP_SIZE = 64;

// Forward declarations
struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64 FenceValue;
};

struct AudioState
{
    float frequency = 440.0f;
    float phase = 0.0f;
};

struct ExampleDescriptorHeapAllocator
{
    ID3D12DescriptorHeap* Heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
    UINT HeapHandleIncrement;
    ImVector<int> FreeIndices;

    void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap);
    void Destroy();
    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle);
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle);
};

// External variable declarations (defined in main.cpp)
extern FrameContext g_frameContext[APP_NUM_FRAMES_IN_FLIGHT];
extern UINT g_frameIndex;
extern ID3D12Device* g_pd3dDevice;
extern ID3D12DescriptorHeap* g_pd3dRtvDescHeap;
extern ID3D12DescriptorHeap* g_pd3dSrvDescHeap;
extern ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;
extern ID3D12CommandQueue* g_pd3dCommandQueue;
extern ID3D12GraphicsCommandList* g_pd3dCommandList;
extern ID3D12Fence* g_fence;
extern HANDLE g_fenceEvent;
extern UINT64 g_fenceLastSignaledValue;
extern IDXGISwapChain3* g_pSwapChain;
extern bool g_SwapChainTearingSupport;
extern bool g_SwapChainOccluded;
extern HANDLE g_hSwapChainWaitableObject;
extern ID3D12Resource* g_mainRenderTargetResource[APP_NUM_BACK_BUFFERS];
extern D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[APP_NUM_BACK_BUFFERS];
extern AudioState g_audioState;

// Function declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForPendingOperations();
FrameContext* WaitForNextFrameContext();