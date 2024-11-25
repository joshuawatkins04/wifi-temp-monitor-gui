#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "udp.h"
#include "window.h"
#include "server.h"
#include "config.h"

Config config = {
		.globalTemperature = 0.0,
		.globalHumidity = 0.0,
		.packetCounter = 0,
		.connectionStatus = "Connecting...",
};

Device devices[] = {
	{"ESP_RESPONSE", "", 0, 0},
	{"IOS_RESPONSE", "", 0, 0},
};
int	numDevices = sizeof(devices) / sizeof(devices[0]);

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

	const int discoveryPort = 12345;
	const int maxRetries = 10;
	const int retryInterval = 2;

	printf("Starting device discovery...\n");

	for (int attempt = 1; attempt <= maxRetries; attempt++)
	{
		printf("Broadcasting discovery message (Attempt %d)...\n", attempt);
		sendBroadcast("DISCOVERY_REQUEST", discoveryPort);

		listenForResponses(discoveryPort, devices); 

		int allInitialised = 1;
		for (int i = 0; i < numDevices; i++)
		{
			if (!devices[i].initialised)
			{
				allInitialised = 0;
				break;
			}
		}
		if (allInitialised)
		{
			printf("All devices initialised!\n");
			break;
		}

		printf("Waiting for %d seconds before retrying...\n", retryInterval);
		Sleep(retryInterval * 1000);
	}

	for (int i = 0; i < numDevices; i++)
	{
		printf("%s %s\n", devices[i].id, devices[i].ip ? "initialised" : "not initialised");
	}

	pthread_t server_thread;

	if (pthread_create(&server_thread, NULL, begin_server_thread, NULL) != 0)
	{
		printf("Continuing despite error with creating server thread.\n");
	}

	if (runBatchScript() != 0)
	{
		printf("Continuing despite error with creating batch file.\n");
	}

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