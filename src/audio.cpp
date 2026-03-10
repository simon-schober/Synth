#define MINIAUDIO_IMPLEMENTATION  // Add this line!
#include "audio.h"
#include "helpers.h"
#include <cmath>
#include <cstdio>

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    float* pOutputF32 = (float*)pOutput;
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        float sample = 0.2f * sinf(2.0f * 3.14159265359f * g_audioState.frequency * g_audioState.phase);
        for (ma_uint32 ch = 0; ch < pDevice->playback.channels; ++ch) {
            *pOutputF32++ = sample;
        }
        g_audioState.phase += 1.0f / (float)DEVICE_SAMPLE_RATE;
        if (g_audioState.phase >= 1.0f) g_audioState.phase -= 1.0f;
    }
    (void)pInput;
}

bool InitializeAudio(ma_device& device)
{
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = DEVICE_FORMAT;
    config.playback.channels = DEVICE_CHANNELS;
    config.sampleRate = DEVICE_SAMPLE_RATE;
    config.dataCallback = data_callback;

    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        return false;
    }

    printf("Audio device initialized. Starting playback...\n");
    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start device.\n");
        ma_device_uninit(&device);
        return false;
    }

    return true;
}

void CleanupAudio(ma_device& device)
{
    ma_device_uninit(&device);
}