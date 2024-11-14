#include <stdio.h>
#include "serial.h"
#include "window.h"
#include "resource.h"

#define WINDOW_TITLE "Temperature Monitor"

extern HFONT hFont;
extern HANDLE hSerial;

HWND createWindow(HINSTANCE hInstance, int nCmdShow) {
    const char CLASS_NAME[] = "TemperatureWindow";
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hIconSm = wc.hIcon;

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window failed to create", "Error", MB_OK | MB_ICONERROR);
        return NULL;
    }

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 150,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, "Window failed to create", "Error", MB_OK | MB_ICONERROR);
        return NULL;
    }

    ShowWindow(hwnd, nCmdShow);
    return hwnd;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            SetBkColor(hdc, RGB(240, 240, 240));
            SelectObject(hdc, hFont);

            float temperature = readTemperature();
            char tempText[64];
            sprintf(tempText, "Temperature: %.2f C", temperature);

            SIZE textSize;
            GetTextExtentPoint32(hdc, tempText, strlen(tempText), &textSize);

            int x = (ps.rcPaint.right - textSize.cx) / 2;
            int y = (ps.rcPaint.bottom - textSize.cy) / 2;
            TextOut(hdc, x, y, tempText, strlen(tempText));

            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            CloseHandle(hSerial);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}