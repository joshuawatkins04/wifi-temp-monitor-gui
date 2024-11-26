#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "windows.h"
#include "udp.h"
#include "logs.h"
#include "config.h"

#pragma comment(lib, "ws2_32.lib")

extern pthread_mutex_t devicesMutex;
extern int allDevicesInitialised;

HANDLE hSerial;

void initWinsock()
{
	static int initialised = 0;
	if (!initialised)
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			printf("Failed to initialise Winsock. Error: %d\n", WSAGetLastError());
		}
		initialised = 1;
	}
}

void sendDirect(const char *message, const char *targetIP, int port)
{
	initWinsock();

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		printf("Failed to create socket. Error: %d\n", WSAGetLastError());
		return;
	}

	struct sockaddr_in targetAddr = {
			.sin_family = AF_INET,
			.sin_port = htons(port),
			.sin_addr.s_addr = inet_addr(targetIP),
	};

	if (sendto(sock, message, strlen(message), 0, (struct sockaddr *)&targetAddr, sizeof(targetAddr)) == SOCKET_ERROR)
	{
		printf("Failed to send packet to %s:%d. Error: %d\n", targetIP, port, WSAGetLastError());
	}
	else
	{
		printf("Discovery request sent: %s:%d\n", targetIP, port);
	}

	closesocket(sock);
}

void listenForResponses(int port, Device devices[])
{
	printf("[DEBUG] Starting listenForResponses on port %d...\n", port);
	initWinsock();

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		printf("Failed to create socket. Error: %d\n", WSAGetLastError());
		return;
	}

	struct sockaddr_in serverAddr = {
			.sin_family = AF_INET,
			.sin_port = htons(port),
			.sin_addr.s_addr = INADDR_ANY,
	};

	if (bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		printf("Failed to bind socket. Error: %d\n", WSAGetLastError());
		closesocket(sock);
		return;
	}

	printf("Listening for responses on port %d...\n", port);

	u_long mode = 1;
	ioctlsocket(sock, FIONBIO, &mode);
	char buffer[256];
	struct sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);

	while (!allDevicesInitialised)
	{
		int bytesRead = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
		if (bytesRead > 0)
		{
			buffer[bytesRead] = '\0';
			char *clientIP = inet_ntoa(clientAddr.sin_addr);
			int clientPort = ntohs(clientAddr.sin_port);
			printf("Received: %s from %s:%d\n", buffer, clientIP, clientPort);

			pthread_mutex_lock(&devicesMutex);
			int foundDevice = 0;
			for (int i = 0; i < numDevices; i++)
			{
				if (!devices[i].initialised && strstr(buffer, devices[i].id))
				{
					printf("Received response from %s: %s\n", devices[i].id, clientIP);
					strncpy(devices[i].ip, clientIP, sizeof(devices[i].ip) - 1);
					devices[i].ip[sizeof(devices[i].ip) - 1] = '\0';
					devices[i].port = clientPort;
					devices[i].initialised = 1;
					printf("Device %s initialised at %s:%d\n", devices[i].id, clientIP, devices[i].port);
					foundDevice = 1;
					break;
				}
			}
			int initializedCount = 0;
			for (int i = 0; i < numDevices; i++)
			{
				if (devices[i].initialised)
				{
					initializedCount++;
				}
			}
			allDevicesInitialised = (initializedCount == numDevices);

			pthread_mutex_unlock(&devicesMutex);
		}
		else if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			printf("[ERROR] recvfrom failed. Error: %d\n", WSAGetLastError());
		}
		Sleep(100);
	}
	closesocket(sock);
}

void readDhtData(float *temperature, float *humidity, Device devices[])
{
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		printf("Failed to create socket. Error: %d\n", WSAGetLastError());
		return;
	}

	struct sockaddr_in bindAddr = {
			.sin_family = AF_INET,
			.sin_port = htons(12345), // Use the port where the data will be sent
			.sin_addr.s_addr = INADDR_ANY,
	};

	if (bind(sock, (struct sockaddr *)&bindAddr, sizeof(bindAddr)) < 0)
	{
		printf("Failed to bind socket. Error: %d\n", WSAGetLastError());
		closesocket(sock);
		return;
	}

	char buffer[32];
	struct sockaddr_in serverAddr;
	int serverAddrLen = sizeof(serverAddr);
	int bytesRead = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&serverAddr, &serverAddrLen);

	if (bytesRead == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK)
		{
			printf("recvfrom failed with error: %d\n", error);
			strncpy(config.connectionStatus, "Error", sizeof(config.connectionStatus) - 1);
		}
		closesocket(sock);
		return;
	}

	buffer[bytesRead] = '\0';
	for (int i = 0; i < numDevices; i++)
	{
		if (devices[i].initialised)
		{
			sendPacket(devices[i].ip, 12345, buffer);
		}
	}
	char *splitData = strchr(buffer, ',');
	if (splitData == NULL)
	{
		printf("Insufficient data received.\n");
		strncpy(config.connectionStatus, "Insufficient data", sizeof(config.connectionStatus) - 1);
		closesocket(sock);
		return;
	}

	*splitData = '\0';
	*temperature = atof(buffer);
	*humidity = atof(splitData + 1);

	strncpy(config.connectionStatus, "Connected", sizeof(config.connectionStatus) - 1);
	config.packetCounter++;

	createLog(*temperature, *humidity);
	closesocket(sock);
}

void sendPacket(const char *ip, int port, const char *message)
{
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		printf("Failed to create socket for sendPacket. Error: %d\n", WSAGetLastError());
		return;
	}

	struct sockaddr_in destAddr =
			{
					destAddr.sin_family = AF_INET,
					destAddr.sin_port = htons(port),
					destAddr.sin_addr.s_addr = inet_addr(ip),
			};

	if (inet_addr(ip) == INADDR_NONE)
	{
		printf("Invalid destination IP: %s\n", ip);
		closesocket(sock);
		return;
	}

	if (sendto(sock, message, strlen(message), 0, (struct sockaddr *)&destAddr, sizeof(destAddr)) == SOCKET_ERROR)
	{
		printf("Send failed. Error: %d\n", WSAGetLastError());
	}
	else
	{
		printf("Message sent: %s to %s:%d\n", message, ip, port);
	}
	closesocket(sock);
}