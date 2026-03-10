#pragma once

#include <Windows.h>

struct WindowContext
{
    HWND hwnd;
    WNDCLASSEXW wc;
    float dpi_scale;
};

// Window initialization and cleanup
bool CreateApplicationWindow(WindowContext& context, int width = 1280, int height = 800);
void DestroyApplicationWindow(const WindowContext& context);

// Message processing
bool ProcessWindowMessages(bool& done);

// Window procedure
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);