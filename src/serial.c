#include "serial.h"
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

HANDLE hSerial;

SOCKET udpSocket;
struct sockaddr_in serverAddr, clientAddr;

void initUDP(const char *ip, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
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

float readTemperature() {
    char buffer[32];
    int clientAddrLen = sizeof(clientAddr);
    int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer) -1, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (bytesRead == SOCKET_ERROR) {
        perror("recvfrom failed");
        return 0.0;
    }

    buffer[bytesRead] = '\0';
    return atof(buffer);
}