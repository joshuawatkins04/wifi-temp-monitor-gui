#include <stdio.h>
#include <pthread.h>
#include "udp.h"
#include "window.h"
#include "config.h"

extern HFONT hFont;
extern HANDLE hSerial;
extern pthread_mutex_t configMutex;
extern volatile int running;

HWND createWindow(HINSTANCE hInstance, int nCmdShow)
{
	const char CLASS_NAME[] = "TemperatureWindow";
	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = CLASS_NAME;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hIconSm = wc.hIcon;

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window failed to create", "Error", MB_OK | MB_ICONERROR);
		return NULL;
	}

	HWND hwnd = CreateWindowEx(
			0, CLASS_NAME, WINDOW_TITLE, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
			CW_USEDEFAULT, CW_USEDEFAULT, 350, 160,
			NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, "Window failed to create", "Error", MB_OK | MB_ICONERROR);
		return NULL;
	}

	ShowWindow(hwnd, nCmdShow);
	return hwnd;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		HDC hdcMem = CreateCompatibleDC(hdc);
		RECT clientRect;
		GetClientRect(hwnd, &clientRect);
		HBITMAP hbmMem = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
		HGDIOBJ hOld = SelectObject(hdcMem, hbmMem);
		FillRect(hdcMem, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		SetBkColor(hdcMem, RGB(240, 240, 240));
		SelectObject(hdcMem, hFont);

		SIZE textSize;
		int screenWidth = clientRect.right - clientRect.left;
		int y = 10;
		char buffer[128];

		pthread_mutex_lock(&configMutex);
		sprintf(buffer, "Packets: %d | %s", config.packetCounter, config.connectionStatus);
		GetTextExtentPoint32(hdcMem, buffer, strlen(buffer), &textSize);
		int x = (screenWidth - textSize.cx) / 2;
		TextOut(hdcMem, x, y, buffer, strlen(buffer));

		sprintf(buffer, "Temperature: %.2f C", config.globalTemperature);

		GetTextExtentPoint32(hdcMem, buffer, strlen(buffer), &textSize);
		x = (screenWidth - textSize.cx) / 2;
		y += textSize.cy + 10;
		TextOut(hdcMem, x, y, buffer, strlen(buffer));

		sprintf(buffer, "Humidity: %.2f %%", config.globalHumidity);
		pthread_mutex_unlock(&configMutex);
		GetTextExtentPoint32(hdcMem, buffer, strlen(buffer), &textSize);
		x = (screenWidth - textSize.cx) / 2;
		y += textSize.cy + 10;
		TextOut(hdcMem, x, y, buffer, strlen(buffer));

		BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

		SelectObject(hdcMem, hOld);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);
		EndPaint(hwnd, &ps);
		break;
	}
	case WM_ERASEBKGND:
		return 1;
	case WM_CREATE:
		SetTimer(hwnd, 1, 1000, NULL);
		break;
	case WM_TIMER:
		updateData();
		RECT updateRect = {10, 10, 330, 140};
		InvalidateRect(hwnd, &updateRect, TRUE);
		break;
	case WM_DESTROY:
		running = 0;
		WSACleanup();
		CloseHandle(hSerial);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void updateData()
{
	readDhtData(&config.globalTemperature, &config.globalHumidity, devices);
}