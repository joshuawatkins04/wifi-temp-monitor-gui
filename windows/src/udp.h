#ifndef UDP_H
#define UDP_H

#include "config.h"

void sendBroadcast(const char *message, int port);
void listenForResponses(int port, Device devices[]);
void readDhtData(float *temperature, float *humidity, Device devices[]);
void sendPacket(const char *ip, int port, const char *message);

#endif