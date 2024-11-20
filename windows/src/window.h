#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>

HWND createWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void updateData();

#endif