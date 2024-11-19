#include <stdlib.h>
#include <stdio.h>
#include "serial.h"
#include "window.h"
#include "config.h"
#include "MQTTClient.h"

#define ADDRESS "tcp://192.168.0.37:1883"
#define CLIENTID "TemperatureMonitor"
#define TOPIC "home/temperature"
#define QOS 1
#define TIMEOUT 10000L

HFONT hFont;

int runBatchScript()
{
	const char *filePath = "C:\\Users\\joshu\\OneDrive\\Desktop\\Projects\\Arduino\\wifi-temp-monitor-gui\\autostart\\windows_autostart.bat";
	int result = system(filePath);

	if (result == 0)
	{
		printf("Startup batch file ran successfully.\n");
	}
	else
	{
		printf("Failed to run batch file. Error: %d\n", result);
	}

	return result;
}

void publishMqtt(float temperature, float humidity) 
{
	MQTTClient client;
	rc = MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	MQTTClient_conectOptions conn_opts = MQTTClient_connectOptions_initializer;
	if (MQTTClient_connect(client, &conn_opts) != MQTTCLIENT_SUCCESS) 
	{
		printf("Failed to connect to MQTT broker.\n");
		MQTTClient_destroy(&client);
		return;
	}

	char payload[50];
	sprintf(payload, "{\"Temperature\": %.2f, \"Humidity\": %.2f}", temperature, humidity);

	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	pubmsg.payload = payload;
	pubmsg.payloadlen = strlen(paylod);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;

	MQTTClient_deliveryToken token;
	MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
	printf("Published message: %s\n", payload);

	MQTTClient_waitForCompletion(client, token, TIMEOUT);
	MQTTClient_disconnect(client, TIMEOUT);
	MQTTClient_destroy(&client);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Debugging console
	// AllocConsole();
	// freopen("CONOUT$", "w", stdout);
	// freopen("CONOUT$", "w", stderr);

	if (runBatchScript() != 0)
	{
		printf("Continuing dispite error with creating batch file.\n");
	}

	initUDP(ip, port);

	HWND hwnd = createWindow(hInstance, nCmdShow);
	if (!hwnd)
	{
		return -1;
	}

	hFont = CreateFont(
			30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_SWISS, "Arial");

	MSG msg = {0};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DeleteObject(hFont);
}