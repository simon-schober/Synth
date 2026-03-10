#pragma once

#include "imgui.h"
#include <functional>

// Forward declarations
struct ImGuiIO;

// Synth UI functions
void CreateNoteInputInterface(bool& show_demo_window, bool& show_another_window, float& f, ImVec4& clear_color, int& counter, ImGuiIO& io);
void CreateNoteInputOctaveInterface(const std::function<float(int, int)>& get_frequency, const char* const note_names[12], float& f);