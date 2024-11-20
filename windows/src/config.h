#ifndef CONFIG_H
#define CONFIG_H

#define WINDOW_TITLE "Temperature Monitor"
#define IDI_ICON1 101

typedef struct
{
  const char *ip;
  int port;
  float globalTemperature;
  float globalHumidity;
  int packetCounter;
  char connectionStatus[24];
} Config;

extern Config config;

#endif