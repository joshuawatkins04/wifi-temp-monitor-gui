#ifndef UDP_H
#define UDP_H

#include <winsock2.h>
#include "config.h"

void initSocket(int port);
void initWinsock();
void listenForResponses(int port, Device devices[]);
void readDhtData(float *temperature, float *humidity, Device devices[]);
void sendPacket(const char *message, const char *ip, int port);
int receivePacket(char *buffer, int bufferSize, struct sockaddr_in *clientAddr);
SOCKET getSocket();
void cleanSocket();

#endif