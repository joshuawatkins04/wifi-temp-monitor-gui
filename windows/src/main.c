#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "udp.h"
#include "window.h"
#include "server.h"
#include "config.h"

#define DISCORVERY_PORT 12345
#define LOCAL_IP "192.168.0.37"
#define SUBNET "192.168.0"
#define START_IP 1
#define END_IP 254

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
int numDevices = sizeof(devices) / sizeof(devices[0]);
pthread_mutex_t devicesMutex = PTHREAD_MUTEX_INITIALIZER;
int allDevicesInitialised = 0;

HFONT hFont;

void *sendPackets(void *args)
{
	while (!allDevicesInitialised)
	{
		for (int i = START_IP; i <= END_IP; i++)
		{
			char targetIP[16];
			snprintf(targetIP, sizeof(targetIP), "%s.%d", SUBNET, i);
			if (strcmp(targetIP, LOCAL_IP) != 0)
			{
				sendDirect("DISCOVERY_REQUEST", targetIP, DISCORVERY_PORT);
			}
			Sleep(100);
		}
		Sleep(500);
	}
	printf("[DEBUG] All devices initialized. Stopping sendPackets thread.\n");
	return NULL;
}

void *listenResponses(void *args)
{
	while (!allDevicesInitialised)
	{
		listenForResponses(DISCORVERY_PORT, devices);
		pthread_mutex_lock(&devicesMutex);
		int initialisedCount = 0;
		for (int i = 0; i < numDevices; i++)
		{
			if (devices[i].initialised)
			{
				initialisedCount++;
			}
		}
		allDevicesInitialised = (initialisedCount == numDevices);
		pthread_mutex_unlock(&devicesMutex);
	}
	return NULL;
}

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

	printf("Starting device discovery...\n");

	pthread_t senderThread, listenerThread;
	pthread_create(&senderThread, NULL, sendPackets, NULL);
	pthread_create(&listenerThread, NULL, listenResponses, NULL);
	pthread_join(senderThread, NULL);
	pthread_join(listenerThread, NULL);

	for (int i = 0; i < numDevices; i++)
	{
		if (devices[i].initialised)
		{
			printf("Device %s found at %s:%d\n", devices[i].id, devices[i].ip, devices[i].port);
		}
		else
		{
			printf("Device %s not found.\n", devices[i].id);
		}
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