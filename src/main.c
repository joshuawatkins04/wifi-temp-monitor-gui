#include <windows.h>
#include "serial.h"
#include "window.h"
#include "resource.h"

HFONT hFont;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char *portName = "COM3";
    initSerial(portName);

    HWND hwnd = createWindow(hInstance, nCmdShow);
    if (!hwnd) {
        return -1;
    }

    hFont = CreateFont(
        30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Arial"
    );

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(hFont);
}