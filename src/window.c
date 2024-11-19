#include <stdio.h>
#include "serial.h"
#include "window.h"
#include "resource.h"

#define WINDOW_TITLE "Temperature Monitor"

static float globalTemperature = 0.0;
static float globalHumidity = 0.0;
int packetCounter = 0;
char connectionStatus[64] = "Connecting...";

extern HFONT hFont;
extern HANDLE hSerial;

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
		SetBkColor(hdc, RGB(240, 240, 240));
		SelectObject(hdc, hFont);

		SIZE textSize;
		RECT clientRect;
		GetClientRect(hwnd, &clientRect);

		char buffer[128];
		int screenWidth = clientRect.right - clientRect.left;
		int y = 10;

		sprintf(buffer, "Packets: %d | %s", packetCounter, connectionStatus);
		GetTextExtentPoint32(hdc, buffer, strlen(buffer), &textSize);
		int x = (screenWidth - textSize.cx) / 2;
		TextOut(hdc, x, y, buffer, strlen(buffer));

		sprintf(buffer, "Temperature: %.2f C", globalTemperature);
		GetTextExtentPoint32(hdc, buffer, strlen(buffer), &textSize);
		x = (screenWidth - textSize.cx) / 2;
		y += textSize.cy + 10;
		TextOut(hdc, x, y, buffer, strlen(buffer));

		sprintf(buffer, "Humidity: %.2f %%", globalHumidity);
		GetTextExtentPoint32(hdc, buffer, strlen(buffer), &textSize);
		x = (screenWidth - textSize.cx) / 2;
		y += textSize.cy + 10;
		TextOut(hdc, x, y, buffer, strlen(buffer));

		EndPaint(hwnd, &ps);
		break;
	}
	case WM_CREATE:
		SetTimer(hwnd, 1, 1000, NULL);
		break;
	case WM_TIMER:
		updateData();
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	case WM_DESTROY:
		WSACleanup();
		CloseHandle(hSerial);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void updateData()
{
	readDhtData(&globalTemperature, &globalHumidity);
}