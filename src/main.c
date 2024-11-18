#include <stdlib.h>
#include <stdio.h>
#include "serial.h"
#include "window.h"
#include "resource.h"

HFONT hFont;

int runBatchScript()
{
	const char *filePath = "C:\\Users\\joshu\\OneDrive\\Desktop\\Projects\\Arduino\\wifi-temp-monitor-gui\\autostart\\windows_autostart.bat";
	int result = system(filePath);

	if (result == 0)
	{
		printf("Startup batch file ran successfully.\n");
	}
	else
	{
		printf("Failed to run batch file. Error: %d\n", result);
	}

	return result;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Debugging console
	// AllocConsole();
	// freopen("CONOUT$", "w", stdout);
	// freopen("CONOUT$", "w", stderr);

	if (runBatchScript() != 0)
	{
		printf("Continuing dispite error with creating batch file.\n");
	}

	const char *ip = "192.168.0.37";
	int port = 5005;
	initUDP(ip, port);

	HWND hwnd = createWindow(hInstance, nCmdShow);
	if (!hwnd)
	{
		return -1;
	}

	hFont = CreateFont(
			30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_SWISS, "Arial");

	MSG msg = {0};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DeleteObject(hFont);
}