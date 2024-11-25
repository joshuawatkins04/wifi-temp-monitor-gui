#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "windows.h"
#include "udp.h"
#include "logs.h"
#include "config.h"

#pragma comment(lib, "ws2_32.lib")

SOCKET udpSocket;
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

void sendBroadcast(const char *message, int port)
{
	initWinsock();

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		printf("Failed to create socket. Error: %d\n", WSAGetLastError());
		return;
	}

	int broadcast = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(broadcast)) < 0)
	{
		printf("Failed to set broadcast option. Error: %d\n", WSAGetLastError());
		closesocket(sock);
		return;
	}

	struct sockaddr_in broadcastAddr = {
			.sin_family = AF_INET,
			.sin_port = htons(port),
			.sin_addr.s_addr = INADDR_BROADCAST,
	};

	if (sendto(sock, message, strlen(message), 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) == SOCKET_ERROR)
	{
		printf("Failed to send broadcast. Error: %d\n", WSAGetLastError());
	}
	else
	{
		printf("Broadcast message sent: %s\n", message);
	}

	closesocket(sock);
}

void listenForResponses(int port, Device devices[])
{
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

	u_long mode = 1;
	ioctlsocket(sock, FIONBIO, &mode);
	time_t startTime = time(NULL);
	while (time(NULL) - startTime < 2)
	{
		struct sockaddr_in clientAddr;
		char buffer[256];
		int clientAddrLen = sizeof(clientAddr);
		int bytesRead = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
		if (bytesRead > 0)
		{
			buffer[bytesRead] = '\0';
			char *clientIP = inet_ntoa(clientAddr.sin_addr);
			int clientPort = ntohs(clientAddr.sin_port);

			for (int i = 0; i < numDevices; i++)
			{
				if (!devices[i].initialised && strstr(buffer, devices[i].id))
				{
					printf("Received response from %s: %s\n", devices[i].id, clientIP);
					strncpy(devices[i].ip, clientIP, sizeof(devices[i].ip) - 1);
					devices[i].ip[sizeof(devices[i].ip) - 1] = '\0';
					devices[i].port =- clientPort;
					devices[i].initialised = 1;
				}
			}
		}
	}
	closesocket(sock);
}

void readDhtData(float *temperature, float *humidity, Device devices[])
{
	char buffer[32];
	struct sockaddr_in serverAddr;
	int serverAddrLen = sizeof(serverAddr);
	int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&serverAddr, &serverAddrLen);

	if (bytesRead == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK)
		{
			printf("recvfrom failed with error: %d\n", error);
			strncpy(config.connectionStatus, "Error", sizeof(config.connectionStatus) - 1);
		}
		return;
	}

	buffer[bytesRead] = '\0';
	for (int i = 0; i < numDevices; i++)
	{
		if (devices[i].initialised)
		{
			sendPacket(devices[i].ip, devices[i].port, buffer);
		}
	}
	char *splitData = strchr(buffer, ',');
	if (splitData == NULL)
	{
		printf("Insufficient data received.\n");
		strncpy(config.connectionStatus, "Insufficient data", sizeof(config.connectionStatus) - 1);
		return;
	}

	*splitData = '\0';
	*temperature = atof(buffer);
	*humidity = atof(splitData + 1);

	strncpy(config.connectionStatus, "Connected", sizeof(config.connectionStatus) - 1);
	config.packetCounter++;

	createLog(*temperature, *humidity);
}

void sendPacket(const char *ip, int port, const char *message)
{
	struct sockaddr_in destAddr =
			{
					destAddr.sin_family = AF_INET,
					destAddr.sin_port = htons(port),
					destAddr.sin_addr.s_addr = inet_addr(ip),
			};

	if (sendto(udpSocket, message, strlen(message), 0, (struct sockaddr *)&destAddr, sizeof(destAddr)) == SOCKET_ERROR)
	{
		printf("Send failed. Error: %d\n", WSAGetLastError());
	}
	else
	{
		printf("Message sent: %s\n", message);
	}
}