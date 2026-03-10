#include "synth_interface.h"
#include "graphics_setup.h"
#include "imgui.h"
#include <cmath>

void CreateNoteInputInterface(bool& show_demo_window, bool& show_another_window, float& f, ImVec4& clear_color, int& counter, ImGuiIO& io)
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
    CreateNoteInputOctaveInterface(get_frequency, note_names, f);

    ImGui::Separator();

    // Manual frequency slider
    ImGui::Text("Manual Control:");
    if (ImGui::SliderFloat("Frequency (Hz)", &f, 20.0f, 2000.0f))
    {
        g_audioState.frequency = f;
    }

    ImGui::End();
}

void CreateNoteInputOctaveInterface(const std::function<float(int, int)>& get_frequency, const char* const note_names[12], float& f)
{
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
}