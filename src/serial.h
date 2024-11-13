#ifndef SERIAL_H
#define SERIAL_H

#include <windows.h>

void initSerial(const char *portName);
float readTemperature();

#endif