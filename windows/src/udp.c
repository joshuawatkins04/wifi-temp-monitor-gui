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
extern pthread_mutex_t socketMutex;
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

	u_long mode = 1;
	ioctlsocket(udpSocket, FIONBIO, &mode);

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

void sendPacket(const char *message, const char *ip, int port)
{
	SOCKET sock = getSocket();
	if (sock == INVALID_SOCKET)
	{
		printf("[FUNCTION - sendPacket] Failed to create socket for sendPacket. Error: %d\n", WSAGetLastError());
		return;
	}

	struct sockaddr_in destAddr =
			{
					destAddr.sin_family = AF_INET,
					destAddr.sin_port = htons(port),
					destAddr.sin_addr.s_addr = inet_addr(ip),
			};

	pthread_mutex_lock(&socketMutex);
	int result = sendto(sock, message, strlen(message), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
	pthread_mutex_unlock(&socketMutex);
	if (result == SOCKET_ERROR)
	{
		printf("[FUNCTION - sendPacket] Send failed. Error: %d\n", WSAGetLastError());
	}
	else
	{
		printf("[FUNCTION - sendPacket] Message sent: %s to %s:%d\n", message, ip, port);
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

	pthread_mutex_lock(&socketMutex);
	int bytesRead = recvfrom(sock, buffer, bufferSize - 1, 0, (struct sockaddr *)clientAddr, &clientAddrLen);
	pthread_mutex_unlock(&socketMutex);

	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		return bytesRead;
	}
	else if (WSAGetLastError() == WSAEWOULDBLOCK)
	{
		return -1;
	}
	else
	{
		printf("[FUNCTION - receivePacket] recvfrom failed. Error: %d\n", WSAGetLastError());
		return -1;
	}
}