#include "serial.h"
#include "windows.h"
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

extern char connectionStatus[64];
extern int packetCounter;

HANDLE hSerial;

SOCKET udpSocket;
struct sockaddr_in serverAddr, clientAddr;

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
	int clientAddrLen = sizeof(clientAddr);
	int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);

	if (bytesRead == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK)
		{
			printf("recvfrom failed with error: %d\n", error);
			strcpy(connectionStatus, "Error");
		}
		else
		{
			printf("No new data received.");
		}
		return;
	}

	buffer[bytesRead] = '\0';
	char *splitData = strchr(buffer, ',');
	if (splitData == NULL)
	{
		printf("Insufficient data received.\n");
		strcpy(connectionStatus, "Insufficient data");
		return;
	}

	*splitData = '\0';
	*temperature = atof(buffer);
	*humidity = atof(splitData + 1);

	strcpy(connectionStatus, "Connected");
  packetCounter++;
}