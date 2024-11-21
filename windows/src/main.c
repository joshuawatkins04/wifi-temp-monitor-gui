#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "udp.h"
#include "window.h"
#include "server.h"
#include "config.h"

Config config = {
		.ip = "192.168.0.37",
		.port = 5005,
		.phoneIP = "192.168.0.161",
		.phonePort = 8082,
		.globalTemperature = 0.0,
		.globalHumidity = 0.0,
		.packetCounter = 0,
		.connectionStatus = "Connecting..."};

HFONT hFont;

int runBatchScript()
{
	const char *filePath = "C:\\Users\\joshu\\OneDrive\\Desktop\\Projects\\Arduino\\wifi-temp-monitor-gui\\windows\\windows_autostart.bat";
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

void *begin_server_thread(void *arg)
{
	start_server();
	return NULL;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Debugging console
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	pthread_t server_thread;

	if (pthread_create(&server_thread, NULL, begin_server_thread, NULL) != 0)
	{
		printf("Continuing dispite error with creating server thread.\n");
	}

	if (runBatchScript() != 0)
	{
		printf("Continuing dispite error with creating batch file.\n");
	}

	initUDP(config.ip, config.port);

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