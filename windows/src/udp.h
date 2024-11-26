#ifndef UDP_H
#define UDP_H

#include "config.h"

void sendDirect(const char *message, const char *targetIP, int port);
void listenForResponses(int port, Device devices[]);
void readDhtData(float *temperature, float *humidity, Device devices[]);
void sendPacket(const char *ip, int port, const char *message);

#endif