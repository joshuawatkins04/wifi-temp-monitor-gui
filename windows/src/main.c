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
		{"ESP_RESPONSE", "", 0, DEVICE_UNINITIALISED},
		{"IOS_RESPONSE", "", 0, DEVICE_UNINITIALISED},
};

int numDevices = sizeof(devices) / sizeof(devices[0]);
pthread_mutex_t devicesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t configMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t allDevicesInitialisedMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t allDevicesInitialisedCond = PTHREAD_COND_INITIALIZER;
pthread_t senderThread, listenerThread, updaterThread, heartbeaterThread, serverThread;
volatile int running = 1;
int allDevicesInitialised = 0;

HFONT hFont;

void *sendThread(void *args)
{
	printf("Starting send thread...\n");
	while (running)
	{
		pthread_mutex_lock(&allDevicesInitialisedMutex);
		if (allDevicesInitialised)
		{
			pthread_mutex_unlock(&allDevicesInitialisedMutex);
			break;
		}
		pthread_mutex_unlock(&allDevicesInitialisedMutex);

		for (int i = START_IP; i <= END_IP; i++)
		{
			char targetIP[16];
			snprintf(targetIP, sizeof(targetIP), "%s.%d", SUBNET, i);
			if (strcmp(targetIP, LOCAL_IP) != 0)
			{
				sendPacket("DISCOVERY_REQUEST", targetIP, DISCORVERY_PORT);
			}
			Sleep(100);
		}
		Sleep(500);
	}
	printf("Stopping sendPackets thread...\n");
	return NULL;
}

void *listenThread(void *args)
{
	printf("Starting listen thread...\n");
	while (running)
	{
		pthread_mutex_lock(&allDevicesInitialisedMutex);
		if (allDevicesInitialised)
		{
			pthread_mutex_unlock(&allDevicesInitialisedMutex);
			break;
		}
		pthread_mutex_unlock(&allDevicesInitialisedMutex);
		
		listenForResponses(DISCORVERY_PORT, devices);
		
		pthread_mutex_lock(&devicesMutex);
		int initialisedCount = 0;
		for (int i = 0; i < numDevices; i++)
		{
			if (devices[i].status == DEVICE_INITIALISED)
			{
				initialisedCount++;
			}
		}
		pthread_mutex_unlock(&devicesMutex);
		if (initialisedCount == numDevices)
		{
			pthread_mutex_lock(&allDevicesInitialisedMutex);
			allDevicesInitialised = 1;
			pthread_cond_signal(&allDevicesInitialisedCond);
			pthread_mutex_lock(&allDevicesInitialisedMutex);
			break;
		}
	}
	printf("Stopping listenResponses thread...\n");
	return NULL;
}

void *updateThread(void *args)
{
	printf("Starting update thread...\n");
	HWND hwnd = (HWND)args;
	while (running)
	{
		pthread_mutex_lock(&configMutex);
		readDhtData(&config.globalTemperature, &config.globalHumidity, devices);
		pthread_mutex_unlock(&configMutex);
		InvalidateRect(hwnd, NULL, TRUE);
		Sleep(1000);
	}
	return NULL;
}

void *heartbeatThread(void *args)
{
	printf("Starting heartbeat thread...\n");
	Device *devices = (Device *)args;
	const int heartbeatInterval = 3000;
	int failCount = 0;

	while (running)
	{
		for (int i = 0; i < numDevices; i++)
		{
			if (devices[i].status != DEVICE_INITIALISED)
			{
				continue;
			}

			sendPacket("HEARTBEAT", devices[i].ip, devices[i].port);
			printf("Sent HEARTBEAT to %s:%d\n", devices[i].ip, devices[i].port);

			char buffer[50];
			struct sockaddr_in clientAddr;
			int bytesRead = receivePacket(buffer, sizeof(buffer), &clientAddr);
			if (bytesRead > 0 && strcmp("HEARTBEAR_ACK", buffer) == 0)
			{
				printf("Received HEARTBEAT_ACK from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
				failCount = 0;
			}
			else
			{
				failCount++;
				printf("Missed heartbeat acknowledgment for %s. Missed count: %d\n", devices->id, failCount);

				if (failCount >= 3)
				{
					printf("Too many missed heartbeats. Marking %s as uninitialized.\n", devices->id);
					devices->status = DEVICE_UNINITIALISED;
					break;
				}
			}
		}
		Sleep(heartbeatInterval);
	}
	return NULL;
}

void *webServerThread(void *arg)
{
	start_server();
	return NULL;
}

void startOtherThreads(HWND hwnd)
{
	printf("Starting additional threads...\n");
	if (pthread_create(&updaterThread, NULL, updateThread, (void *)hwnd) != 0)
	{
		printf("Failed to create updaterThread.\n");
	}
	if (pthread_create(&serverThread, NULL, webServerThread, NULL) != 0)
	{
		printf("Continuing despite error with creating server thread.\n");
	}
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Debugging console
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	if (runBatchScript() != 0)
	{
		printf("Continuing despite error with creating batch file.\n");
	}

	printf("Starting device discovery...\n");

	initSocket(12345);

	pthread_create(&senderThread, NULL, sendThread, NULL);
	pthread_create(&listenerThread, NULL, listenThread, NULL);

	HWND hwnd = createWindow(hInstance, nCmdShow);
	if (!hwnd)
	{
		running = 0;
		pthread_join(senderThread, NULL);
		pthread_join(listenerThread, NULL);
		return -1;
	}

	pthread_mutex_lock(&allDevicesInitialisedMutex);
	while (!allDevicesInitialised)
	{
		pthread_cond_wait(&allDevicesInitialisedCond, &allDevicesInitialisedMutex);
	}
	pthread_mutex_unlock(&allDevicesInitialisedMutex);

	startOtherThreads(hwnd);

	// for (int i = 0; i < numDevices; i++)
	// {
	// 	if (devices[i].status == DEVICE_INITIALISED)
	// 	{
	// 		printf("Device %s found at %s:%d\n", devices[i].id, devices[i].ip, devices[i].port);
	// 		pthread_create(&heartbeaterThread, NULL, heartbeatThread, NULL);
	// 		pthread_detach(heartbeaterThread);
	// 	}
	// 	else
	// 	{
	// 		printf("Device %s not found.\n", devices[i].id);
	// 	}
	// }

	// if (pthread_create(&serverThread, NULL, webServerThread, NULL) != 0)
	// {
	// 	printf("Continuing despite error with creating server thread.\n");
	// }

	// if (runBatchScript() != 0)
	// {
	// 	printf("Continuing despite error with creating batch file.\n");
	// }

	
	//pthread_create(&updaterThread, NULL, updateThread, (void *)hwnd);

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

	running = 0;
	pthread_join(senderThread, NULL);
	pthread_join(listenerThread, NULL);
	pthread_cancel(serverThread);
	pthread_cancel(updaterThread);
	cleanSocket();

	DeleteObject(hFont);
}