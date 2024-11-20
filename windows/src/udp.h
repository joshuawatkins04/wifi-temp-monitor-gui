#ifndef UDP_H
#define UDP_H

void initUDP(const char *ip, int port);
void readDhtData(float *temperature, float *humidity);
void sendPacket(const char *ip, int port, const char *message);

#endif