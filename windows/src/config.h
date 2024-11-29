#ifndef CONFIG_H
#define CONFIG_H

#define WINDOW_TITLE "Temperature Monitor"
#define IDI_ICON1 101

typedef enum {
  DEVICE_UNINITIALISED = 0,
  DEVICE_INITIALISED = 1
} DeviceStatus;

typedef struct
{
  float globalTemperature;
  float globalHumidity;
  int packetCounter;
  char connectionStatus[24];
} Config;

typedef struct
{
  char id[32];
  char ip[16];
  int port;
  DeviceStatus status;
  int failCount;
} Device;

extern Config config;
extern Device devices[];
extern int numDevices;

#endif