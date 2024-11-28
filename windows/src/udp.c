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
extern pthread_mutex_t configMutex;
extern int allDevicesInitialised;

static SOCKET udpSocket = INVALID_SOCKET;

HANDLE hSerial;

void initSocket(int port)
{
	initWinsock();

	udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpSocket == INVALID_SOCKET)
	{
		printf("Failed to create socket. Error: %d\n", WSAGetLastError());
		return;
	}

	struct sockaddr_in serverAddr = {
			.sin_family = AF_INET,
			.sin_port = htons(port),
			.sin_addr.s_addr = INADDR_ANY,
	};

	if (bind(udpSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		printf("Failed to bind socket. Error: %d\n", WSAGetLastError());
		closesocket(udpSocket);
		udpSocket = INVALID_SOCKET;
		return;
	}

	printf("UDP socket initialised and bound to port %d\n", port);
}

SOCKET getSocket()
{
	return udpSocket;
}

void cleanSocket()
{
	if (udpSocket != INVALID_SOCKET)
	{
		closesocket(udpSocket);
		udpSocket = INVALID_SOCKET;
		printf("Socket cleaned up.\n");
	}
}

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

void listenForResponses(int port, Device devices[])
{
	printf("Starting listenForResponses...\n");

	SOCKET sock = getSocket();
	if (sock == INVALID_SOCKET)
	{
		printf("Failed to create socket. Error: %d\n", WSAGetLastError());
		return;
	}

	// struct sockaddr_in serverAddr = {
	// 		.sin_family = AF_INET,
	// 		.sin_port = htons(port),
	// 		.sin_addr.s_addr = INADDR_ANY,
	// };

	// if (bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	// {
	// 	printf("Failed to bind listenForResponses socket. Error: %d\n", WSAGetLastError());
	// 	closesocket(sock);
	// 	return;
	// }

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
				if (devices[i].status == DEVICE_UNINITIALISED && strstr(buffer, devices[i].id))
				{
					printf("Received response from %s: %s\n", devices[i].id, clientIP);
					strncpy(devices[i].ip, clientIP, sizeof(devices[i].ip) - 1);
					devices[i].ip[sizeof(devices[i].ip) - 1] = '\0';
					devices[i].port = clientPort;
					devices[i].status = DEVICE_INITIALISED;
					printf("Device %s initialised at %s:%d\n", devices[i].id, clientIP, devices[i].port);
					foundDevice = 1;
					break;
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
			allDevicesInitialised = (initialisedCount == numDevices);

			pthread_mutex_unlock(&devicesMutex);
		}
		else if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			printf("recvfrom failed. Error: %d\n", WSAGetLastError());
		}
		Sleep(100);
	}
	printf("Ending listenForResponses...\n");
}

void readDhtData(float *temperature, float *humidity, Device devices[])
{
	// SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	SOCKET sock = getSocket();
	if (sock == INVALID_SOCKET)
	{
		printf("Failed to create socket. Error: %d\n", WSAGetLastError());
		return;
	}

	// struct sockaddr_in bindAddr = {
	// 		.sin_family = AF_INET,
	// 		.sin_port = htons(12345),
	// 		.sin_addr.s_addr = INADDR_ANY,
	// };

	// if (bind(sock, (struct sockaddr *)&bindAddr, sizeof(bindAddr)) < 0)
	// {
	// 	printf("Failed to bind readDhtData socket. Error: %d\n", WSAGetLastError());
	// 	closesocket(sock);
	// 	return;
	// }

	fd_set readfds;
	struct timeval timeout = {2, 0};
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);

	printf("Waiting for data...\n");
	int activity = select(sock + 1, &readfds, NULL, NULL, &timeout);
	if (activity == 0)
	{
		printf("Timeout: No data received within 2 seconds.\n");
		pthread_mutex_lock(&configMutex);
		strncpy(config.connectionStatus, "Timeout", sizeof(config.connectionStatus) - 1);
		pthread_mutex_unlock(&configMutex);
		closesocket(sock);
		return;
	}
	else if (activity < 0)
	{
		printf("Error during select. Error: %d\n", WSAGetLastError());
		pthread_mutex_lock(&configMutex);
		strncpy(config.connectionStatus, "Error", sizeof(config.connectionStatus) - 1);
		pthread_mutex_unlock(&configMutex);
		closesocket(sock);
		return;
	}

	char buffer[32];
	struct sockaddr_in serverAddr;
	// int serverAddrLen = sizeof(serverAddr);
	// int bytesRead = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&serverAddr, &serverAddrLen);
	int bytesRead = receivePacket(buffer, sizeof(buffer), &serverAddr);
	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		printf("Received data: %s\n", buffer);

		for (int i = 0; i < numDevices; i++)
		{
			if (devices[i].status == DEVICE_INITIALISED)
			{
				sendPacket(buffer, devices[i].ip, 12345);
			}
		}

		char *splitData = strchr(buffer, ',');
		if (splitData != NULL)
		{
			*splitData = '\0';
			*temperature = atof(buffer);
			*humidity = atof(splitData + 1);

			pthread_mutex_lock(&configMutex);
			strncpy(config.connectionStatus, "Connected", sizeof(config.connectionStatus) - 1);
			config.packetCounter++;
			pthread_mutex_unlock(&configMutex);

			createLog(*temperature, *humidity);
		}
	}
	// if (bytesRead == SOCKET_ERROR)
	// {
	// 	printf("recvfrom failed with error: %d\n", WSAGetLastError());
	// 	pthread_mutex_lock(&configMutex);
	// 	strncpy(config.connectionStatus, "Error", sizeof(config.connectionStatus) - 1);
	// 	pthread_mutex_unlock(&configMutex);
	// 	closesocket(sock);
	// 	return;
	// }
	// if (splitData == NULL)
	// {
	// 	printf("Insufficient data received.\n");
	// 	pthread_mutex_lock(&configMutex);
	// 	strncpy(config.connectionStatus, "Insufficient data", sizeof(config.connectionStatus) - 1);
	// 	pthread_mutex_unlock(&configMutex);
	// 	closesocket(sock);
	// 	return;
	// }
}

void sendPacket(const char *message, const char *ip, int port)
{
	SOCKET sock = getSocket();
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

	if (sendto(sock, message, strlen(message), 0, (struct sockaddr *)&destAddr, sizeof(destAddr)) == SOCKET_ERROR)
	{
		printf("Send failed. Error: %d\n", WSAGetLastError());
	}
	else
	{
		printf("Message sent: %s to %s:%d\n", message, ip, port);
	}
}

int receivePacket(char *buffer, int bufferSize, struct sockaddr_in *clientAddr)
{
	SOCKET sock = getSocket();
	if (sock == INVALID_SOCKET)
	{
		printf("Socket is not initialised.\n");
		return -1;
	}

	int clientAddrLen = sizeof(*clientAddr);
	int bytesRead = recvfrom(sock, buffer, bufferSize - 1, 0, (struct sockaddr *)clientAddr, &clientAddrLen);
	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		printf("Received %d bytes from %s:%d\n", bytesRead, inet_ntoa(clientAddr->sin_addr), ntohs(clientAddr->sin_port));
		return bytesRead;
	}
	else if (WSAGetLastError() != WSAEWOULDBLOCK)
	{
		printf("recvfrom in receivePacket() failed. Error: %d\n", WSAGetLastError());
	}
	return -1;
}