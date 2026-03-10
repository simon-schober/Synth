#include "midi.h"
#include <mmeapi.h>
#include <Windows.h>
#include <cstdio>
#pragma comment(lib, "winmm.lib")

static void DebugPrint(const char* format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    OutputDebugStringA(buffer);
}

static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    MidiContext* context = reinterpret_cast<MidiContext*>(dwInstance);
    
    // Debug: print all MIDI messages
    if (wMsg == MIM_DATA)
    {
        BYTE status = dwParam1 & 0xFF;
        BYTE data1 = (dwParam1 >> 8) & 0xFF;  // Note number
        BYTE data2 = (dwParam1 >> 16) & 0xFF; // Velocity
        
        DebugPrint("MIDI Raw: status=0x%02X, data1=%d, data2=%d\n", status, data1, data2);
        
        if (context->callback)
        {
            bool note_on = (status & 0xF0) == 0x90 && data2 > 0;
            bool note_off = (status & 0xF0) == 0x80 || ((status & 0xF0) == 0x90 && data2 == 0);
            
            if (note_on || note_off)
            {
                context->callback(data1, data2, note_on);
            }
        }
    }
    else if (wMsg == MIM_OPEN)
    {
        DebugPrint("MIDI: MIM_OPEN received\n");
    }
    else if (wMsg == MIM_CLOSE)
    {
        DebugPrint("MIDI: MIM_CLOSE received\n");
    }
}

bool InitializeMidi(MidiContext& context, std::function<void(int, int, bool)> callback)
{
    context.callback = callback;
    context.device = nullptr;
    
    UINT numDevices = midiInGetNumDevs();
    DebugPrint("MIDI: Found %d input device(s)\n", numDevices);
    
    if (numDevices == 0)
    {
        DebugPrint("MIDI: No input devices available\n");
        return false;
    }
    
    // List all available devices
    for (UINT i = 0; i < numDevices; i++)
    {
        MIDIINCAPS caps;
        if (midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS)) == MMSYSERR_NOERROR)
        {
            DebugPrint("MIDI Device %d: %s\n", i, caps.szPname);
        }
    }
    
    // Try to open device index 0 (change this if you need a different device)
    UINT deviceId = 1;
    DebugPrint("MIDI: Attempting to open device %d...\n", deviceId);
    
    MMRESULT result = midiInOpen(&context.device, deviceId, (DWORD_PTR)MidiInProc, 
                                  (DWORD_PTR)&context, CALLBACK_FUNCTION);
    
    if (result != MMSYSERR_NOERROR)
    {
        DebugPrint("MIDI: Failed to open device %d (error code: %d)\n", deviceId, result);
        return false;
    }
    
    DebugPrint("MIDI: Device %d opened successfully\n", deviceId);
    
    result = midiInStart(context.device);
    if (result != MMSYSERR_NOERROR)
    {
        DebugPrint("MIDI: Failed to start device (error code: %d)\n", result);
        midiInClose(context.device);
        context.device = nullptr;
        return false;
    }
    
    DebugPrint("MIDI: Device started, listening for input...\n");
    return true;
}

void CleanupMidi(MidiContext& context)
{
    if (context.device)
    {
        midiInStop(context.device);
        midiInClose(context.device);
        context.device = nullptr;
        DebugPrint("MIDI: Device closed\n");
    }
}

