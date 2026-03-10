#pragma once

#include <Windows.h>
#include <functional>

struct MidiContext
{
    HMIDIIN device;
    std::function<void(int note, int velocity, bool note_on)> callback;
};

bool InitializeMidi(MidiContext& context, std::function<void(int, int, bool)> callback);
void CleanupMidi(MidiContext& context);