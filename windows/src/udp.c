#include <winsock2.h>
#include <stdio.h>
#include "windows.h"
#include "udp.h"
#include "config.h"

#pragma comment(lib, "ws2_32.lib")

HANDLE hSerial;

SOCKET udpSocket;
struct sockaddr_in serverAddr, espAddr, iosAddr;

void initUDP(const char *ip, int port)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpSocket == INVALID_SOCKET)
	{
		perror("Socket creation failed");
		return;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(ip);

	bind(udpSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

	u_long mode = 1;
	ioctlsocket(udpSocket, FIONBIO, &mode);
}

void readDhtData(float *temperature, float *humidity)
{
	char buffer[32];
	int serverAddrLen = sizeof(serverAddr);
	int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&serverAddr, &serverAddrLen);

	if (bytesRead == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK)
		{
			printf("recvfrom failed with error: %d\n", error);
			strcpy(config.connectionStatus, "Error");
		}
		return;
	}

	buffer[bytesRead] = '\0';
	sendPacket(config.phoneIP, config.phonePort, buffer);
	char *splitData = strchr(buffer, ',');
	if (splitData == NULL)
	{
		printf("Insufficient data received.\n");
		strcpy(config.connectionStatus, "Insufficient data");
		return;
	}

	*splitData = '\0';
	*temperature = atof(buffer);
	*humidity = atof(splitData + 1);

	strcpy(config.connectionStatus, "Connected");
	config.packetCounter++;
}

void sendPacket(const char *ip, int port, const char *message)
{
	struct sockaddr_in destAddr;
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(port);
	destAddr.sin_addr.s_addr = inet_addr(ip);

	if (sendto(udpSocket, message, strlen(message), 0, (struct sockaddr *)&destAddr, sizeof(destAddr)) == SOCKET_ERROR)
	{
		printf("Send failed. Error: %d\n", WSAGetLastError());
	}
	else
	{
		printf("Message sent: %s\n", message);
	}
}