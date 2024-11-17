#ifndef SERIAL_H
#define SERIAL_H

#include <winsock2.h>

void initUDP(const char *ip, int port);
void readDhtData(float *temperature, float *humidity);

#endif