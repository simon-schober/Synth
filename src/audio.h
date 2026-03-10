//void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
//void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
//bool InitializeAudio(ma_device& device)
//void CleanupAudio(ma_device& device)

#pragma once

#include <miniaudio.h>

#define DEVICE_FORMAT   ma_format_f32
#define DEVICE_CHANNELS 2
#define DEVICE_SAMPLE_RATE 48000

// Forward declaration
struct AudioState;

// Audio callback function
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

// Audio initialization and cleanup
bool InitializeAudio(ma_device& device);
void CleanupAudio(ma_device& device);