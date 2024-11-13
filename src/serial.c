#include "serial.h"
#include <stdio.h>

HANDLE hSerial;

void initSerial(const char *portName) {
    hSerial = CreateFile(portName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);
    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    SetCommState(hSerial, &dcbSerialParams);
}

float readTemperature() {
    char buffer[32];
    DWORD bytesRead;
    if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        buffer[bytesRead] = '\0';
        return atof(buffer);
    }
    return 0.0;
}