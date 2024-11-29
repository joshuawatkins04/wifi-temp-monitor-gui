#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "udp.h"
#include "window.h"
#include "server.h"
#include "logs.h"
#include "config.h"

#define PORT 12345
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
		{"ESP_RESPONSE", "", 0, DEVICE_UNINITIALISED, 0},
		{"IOS_RESPONSE", "", 0, DEVICE_UNINITIALISED, 0},
};

int numDevices = sizeof(devices) / sizeof(devices[0]);
pthread_mutex_t devicesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t configMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t allDevicesInitialisedMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socketMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t allDevicesInitialisedCond = PTHREAD_COND_INITIALIZER;
pthread_t senderThread, listenerThread, updaterThread, heartbeaterThread, serverThread;
volatile int running = 1;
int allDevicesInitialised = 0;

HFONT hFont;

typedef enum
{
	MSG_DISCOVERY_RESPONSE,
	MSG_HEARTBEAT_ACK,
	MSG_TEMPERATURE_DATA,
	MSG_UNKNOWN
} MessageType;

MessageType parseMessage(const char *message);
void handleDiscoveryResponse(const char *message, const struct sockaddr_in *senderAddr);
void handleHeartbeatAck(const char *message, const struct sockaddr_in *senderAddr);
void handleTemperatureData(const char *message, const struct sockaddr_in *senderAddr);

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
				sendPacket("DISCOVERY_REQUEST", targetIP, PORT);
			}
			Sleep(50);

			pthread_mutex_lock(&allDevicesInitialisedMutex);
			if (allDevicesInitialised)
			{
				pthread_mutex_unlock(&allDevicesInitialisedMutex);
				break;
			}
			pthread_mutex_unlock(&allDevicesInitialisedMutex);
		}
		// Sleep(500);
	}
	printf("Stopping sendPackets thread...\n");
	return NULL;
}

void *listenThread(void *args)
{
	printf("[THREAD - listenThread] Starting listen thread...\n");
	while (running)
	{
		char buffer[256];
		struct sockaddr_in senderAddr;
		int bytesRead = receivePacket(buffer, sizeof(buffer), &senderAddr);
		if (bytesRead > 0)
		{
			buffer[bytesRead] = '\0';
			printf("[THREAD - listenThread] Received: %s from %s:%d\n", buffer, inet_ntoa(senderAddr.sin_addr), ntohs(senderAddr.sin_port));
			MessageType msgType = parseMessage(buffer);
			switch (msgType)
			{
			case MSG_DISCOVERY_RESPONSE:
				handleDiscoveryResponse(buffer, &senderAddr);
				break;
			case MSG_HEARTBEAT_ACK:
				handleHeartbeatAck(buffer, &senderAddr);
				break;
			case MSG_TEMPERATURE_DATA:
				handleTemperatureData(buffer, &senderAddr);
				break;
			default:
				printf("[THREAD - listenThread] Unknown message type received.\n");
				break;
			}
		}
		else
		{
			Sleep(100);
		}
	}
	printf("[THREAD - listenThread] Stopping listenResponses thread...\n");
	return NULL;
}

MessageType parseMessage(const char *message)
{
	if (strcmp("ESP_RESPONSE", message) == 0 || strcmp("IOS_RESPONSE", message) == 0)
	{
		return MSG_DISCOVERY_RESPONSE;
	}
	else if (strcmp("HEARTBEAT_ACK", message) == 0)
	{
		return MSG_HEARTBEAT_ACK;
	}
	else if (strchr(message, ',') != NULL)
	{
		return MSG_TEMPERATURE_DATA;
	}
	else
	{
		return MSG_UNKNOWN;
	}
}

void handleDiscoveryResponse(const char *message, const struct sockaddr_in *senderAddr)
{
	pthread_mutex_lock(&devicesMutex);
	for (int i = 0; i < numDevices; i++)
	{
		if (devices[i].status == DEVICE_UNINITIALISED && strcmp(devices[i].id, message) == 0)
		{
			strncpy(devices[i].ip, inet_ntoa(senderAddr->sin_addr), sizeof(devices[i].ip) - 1);
			devices[i].ip[sizeof(devices[i].ip) - 1] = '\0';
			// devices[i].port = ntohs(senderAddr->sin_port);
			devices[i].port = 12345;
			devices[i].status = DEVICE_INITIALISED;
			devices[i].failCount = 0;
			printf("[FUNCTION - handleDiscoveryResponse] Device %s initialised at %s:%d\n", devices[i].id, devices[i].ip, devices[i].port);
		}
	}
	int initialisedCount = 0;
	for (int i = 0; i < numDevices; i++)
	{
		if (devices[i].status == DEVICE_INITIALISED)
		{
			initialisedCount++;
		}
	}
	if (initialisedCount == numDevices)
	{
		pthread_mutex_lock(&allDevicesInitialisedMutex);
		allDevicesInitialised = 1;
		pthread_cond_signal(&allDevicesInitialisedCond);
		pthread_mutex_unlock(&allDevicesInitialisedMutex);
	}
	pthread_mutex_unlock(&devicesMutex);
}

void handleHeartbeatAck(const char *message, const struct sockaddr_in *senderAddr)
{
	pthread_mutex_lock(&devicesMutex);
	for (int i = 0; i < numDevices; i++)
	{
		if (devices[i].status == DEVICE_INITIALISED &&
				strcmp(devices[i].ip, inet_ntoa(senderAddr->sin_addr)) == 0)
		{
			devices[i].failCount = 0;
			printf("[FUNCTION - handleHeartbeatAck] Received HEARTBEAT_ACK from %s\n", devices[i].id);
		}
	}
	pthread_mutex_unlock(&devicesMutex);
}

void handleTemperatureData(const char *message, const struct sockaddr_in *senderAddr)
{
	char tempStr[16], humStr[16];
	sscanf(message, "%[^,],%s", tempStr, humStr);
	float temperature = atof(tempStr);
	float humidity = atof(humStr);

	pthread_mutex_lock(&configMutex);
	config.globalTemperature = temperature;
	config.globalHumidity = humidity;
	strncpy(config.connectionStatus, "Connected", sizeof(config.connectionStatus) - 1);
	config.packetCounter++;
	pthread_mutex_unlock(&configMutex);

	createLog(temperature, humidity);

	pthread_mutex_lock(&devicesMutex);
	for (int i = 0; i < numDevices; i++)
	{
		if (devices[i].status == DEVICE_INITIALISED &&
				strcmp(devices[i].ip, inet_ntoa(senderAddr->sin_addr)) != 0)
		{
			sendPacket(message, devices[i].ip, devices[i].port);
		}
	}
	pthread_mutex_unlock(&devicesMutex);
}

void *updateThread(void *args)
{
	printf("Starting update thread...\n");
	HWND hwnd = (HWND)args;
	while (running)
	{
		InvalidateRect(hwnd, NULL, TRUE);
		Sleep(1000);
	}
	return NULL;
}

void *heartbeatThread(void *args)
{
	printf("[THREAD - heartbeat] Starting heartbeat thread...\n");
	const int heartbeatInterval = 3000;

	while (running)
	{
		pthread_mutex_lock(&devicesMutex);
		for (int i = 0; i < numDevices; i++)
		{
			if (devices[i].status != DEVICE_INITIALISED)
			{
				continue;
			}

			sendPacket("HEARTBEAT", devices[i].ip, devices[i].port);
			printf("[THREAD - heartbeat] Sent HEARTBEAT to %s:%d\n", devices[i].ip, devices[i].port);

			devices[i].failCount++;
			if (devices[i].failCount >= 5)
			{
				printf("[THREAD - heartbeatThread] Too many missed heartbeats. Marking %s as uninitialised.\n", devices[i].status);
				devices[i].status = DEVICE_UNINITIALISED;
			}
		}
		pthread_mutex_unlock(&devicesMutex);
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
	printf("[FUNCTION - startOtherThreads] Starting additional threads...\n");
	if (pthread_create(&updaterThread, NULL, updateThread, (void *)hwnd) != 0)
	{
		printf("[FUNCTION - startOtherThreads] Failed to create updaterThread.\n");
	}
	if (pthread_create(&heartbeaterThread, NULL, heartbeatThread, NULL) != 0)
	{
		printf("[FUNCTION - startOtherThreads] Failed to create heartbeatThread.\n");
	}
	if (pthread_create(&serverThread, NULL, webServerThread, NULL) != 0)
	{
		printf("[FUNCTION - startOtherThreads] Continuing despite error with creating server thread.\n");
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

	initSocket(PORT);

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

	printf("[MAIN] Devices initialised. Starting other threads...\n");
	startOtherThreads(hwnd);

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