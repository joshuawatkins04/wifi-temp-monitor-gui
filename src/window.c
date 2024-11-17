#include <stdio.h>
#include "serial.h"
#include "window.h"
#include "resource.h"

#define WINDOW_TITLE "Temperature Monitor"

static float globalTemperature = 0.0;
static float globalHumidity = 0.0;

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
			0, CLASS_NAME, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 300, 150,
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

		char temperatureText[64], humidityText[64];
		sprintf(temperatureText, "Temperature: %.2f C", globalTemperature);
		sprintf(humidityText, "Humidity: %.2f %%", globalHumidity);

		SIZE textSize;
		GetTextExtentPoint32(hdc, temperatureText, strlen(temperatureText), &textSize);

		int x = (ps.rcPaint.right - textSize.cx) / 2;
		int y = (ps.rcPaint.bottom - textSize.cy) / 4;
		TextOut(hdc, x, y, temperatureText, strlen(temperatureText));

		GetTextExtentPoint32(hdc, humidityText, strlen(humidityText), &textSize);
		y += textSize.cy + 20;
		TextOut(hdc, x, y, humidityText, strlen(humidityText));

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