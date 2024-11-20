#ifndef SERIAL_H
#define SERIAL_H

void initUDP(const char *ip, int port);
void readDhtData(float *temperature, float *humidity);

#endif