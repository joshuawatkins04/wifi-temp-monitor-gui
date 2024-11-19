#ifndef CONFIG_H
#define CONFIG_H
    
#define WINDOW_TITLE "Temperature Monitor"
#define IDI_ICON1 101

// IP related
const char *ip = "192.168.0.37";
int port = 5005;

// Global variables for screen display
static float globalTemperature = 0.0;
static float globalHumidity = 0.0;
int packetCounter = 0;
char connectionStatus[64] = "Connecting...";

#endif