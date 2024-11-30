/* For this program to work, you will need a config.h file with the following:
 *   - const char *ssid = "WIFI NAME";
 *   - const char *password = "WIFI PASSWORD";
 *   - const char *sendAddress = "IP ADDRESS";
 *   - const int sendPort = PORT;
*/

#include <WiFi.h>
#include <WiFiUdp.h>
#include <DHT.h>
#include "config.h"

#define DHT22_PIN 23

WiFiUDP udp;
DHT dht(DHT22_PIN, DHT22);

float currentTemperature, currentHumidity, lastTemperature = 0.0, lastHumidity = 0.0;
bool connected = false;
char incomingPacket[255];
unsigned long lastUpdate = 0;
unsigned long lastHeartbeatUpdate = 0;
unsigned long lastReconnectAttempt = 0;
const unsigned long updateInterval = 10000;
const unsigned long heartbeartInterval = 3000;
const unsigned long reconnectInterval = 5000;
int failCount = 0;

const IPAddress multicastAddress(224, 0, 0, 251);
const unsigned int multicastPort = 5353;
IPAddress iosAppIP;
unsigned int iosAppPort;
bool iosAppConnected = false;
int iosAppFailCount = 0;

void setup() 
{
  Serial.begin(9600);
  dht.begin();
  delay(2000);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
  }

  udp.begin(sendPort);
  // udp.beginMulticast(WiFi.localIP(), multicastAddress, multicastPort);
}

void loop() 
{
  checkWifiConnection();
  receivePacket();

  // if (!connected)
  // {
    handleReconnection();
  // }
  // else
  // {
    // handleHeartbeat();
    handleDataRequest();
    handleSensorData();
  // }
}

void handleDataRequest()
{
  if (strcmp(incomingPacket, "IOS_CONNECT") == 0)
  {
    connected = true;
    iosAppConnected = true;
    iosAppIP = udp.remoteIP();
    iosAppPort = 12345;
    failCount = 0;
    requestSensorData();
    Serial.println("Connected to IOS app via IOS_CONNECT packet");
  }
  else if (strcmp(incomingPacket, "DATA_REQ") == 0)
  {
    requestSensorData();
  }
}

void handleReconnection()
{
  if (!connected)
  {
  unsigned long currentTime = millis();
  if (currentTime - lastReconnectAttempt >= reconnectInterval)
  {
    lastReconnectAttempt = currentTime;
    // sendPacket("DISCOVERY_REQUEST", multicastAddress, multicastPort);
    // Serial.println("Sent DISCOVERY_REQUEST message to multicast group");
  }
  // receivePacket();
  if (strcmp(incomingPacket, "IOS_CONNECT") == 0)
  {
    connected = true;
    iosAppConnected = true;
    iosAppIP = udp.remoteIP();
    iosAppPort = 12345;
    failCount = 0;
    Serial.print("Connected to IOS app via IOS_CONNECT packet. IP: ");
    Serial.print(iosAppIP);
    Serial.print(" Port: ");
    Serial.println(iosAppPort);
  }
  else if (strcmp(incomingPacket, "HEARTBEAT") == 0)
  {
    sendPacket("HEARTBEAT_ACK", sendAddress, sendPort);
    connected = true;
    failCount = 0;
    Serial.println("Sent HEARTBEAT_ACK");
  }
  else if (strcmp(incomingPacket, "IOS_RESPONSE") == 0)
  {
    iosAppConnected = true;
    iosAppFailCount = 0;
    iosAppIP = udp.remoteIP();
    iosAppPort = 12345;
    Serial.println("Connected to ios app.");
  }
  else if (strcmp(incomingPacket, "DISCOVERY_REQUEST") == 0)
  {
    sendPacket("ESP_RESPONSE", sendAddress, sendPort);
    connected = true;
    failCount = 0;
    Serial.println("Connected to server.");
  }
  else
  {
    if (currentTime - lastReconnectAttempt >= reconnectInterval)
    {
      lastReconnectAttempt = currentTime;
      sendPacket("RECONNECT", sendAddress, sendPort);
      Serial.println("Sent RECONNECT message");
    }
  }

  }
}

void handleHeartbeat()
{
  unsigned long currentTime = millis();

  if (currentTime - lastHeartbeatUpdate >= heartbeartInterval)
  {
    lastHeartbeatUpdate = millis();
    receivePacket();
    if (strcmp(incomingPacket, "HEARTBEAT") == 0)
    {
      sendPacket("HEARTBEAT_ACK", sendAddress, sendPort);
      failCount = 0;
      Serial.println("HEARTBEAT received and ACK sent.");
    }
    else
    {
      failCount++;
      Serial.print("HEARTBEAT not received. Failcount: ");
      Serial.println(failCount);
      if (failCount >= 3)
      {
        connected = false;
        failCount = 0;
        Serial.println("Connection lost. Attempting to reconnect.");
      }
    }
    lastHeartbeatUpdate = currentTime;
  }
}

void handleSensorData()
{
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= updateInterval)
  {
    lastUpdate = currentTime;
    if (getDhtReading())
    {
      if (absoluteValue(currentTemperature - lastTemperature) > 0.1 || absoluteValue(currentHumidity - lastHumidity) > 0.2) 
      {
        udp.beginPacket(sendAddress, sendPort);
        udp.printf("%.2f,%.2f", currentTemperature, currentHumidity);
        udp.endPacket();

        if (iosAppConnected)
        {
          udp.beginPacket(iosAppIP, iosAppPort);
          udp.printf("%.2f,%.2f", currentTemperature, currentHumidity);
          udp.endPacket();
        }

        lastTemperature = currentTemperature;
        lastHumidity = currentHumidity;
      }
    }
  }
}

void requestSensorData()
{
  if (getDhtReading())
  {
    Serial.println("Getting reading");
    lastTemperature = currentTemperature;
    lastHumidity = currentHumidity;
  }
  udp.beginPacket(iosAppIP, iosAppPort);
  udp.printf("%.2f,%.2f", currentTemperature, currentHumidity);
  udp.endPacket();
  Serial.printf("%.2f,%.2f", currentTemperature, currentHumidity);
}

void checkWifiConnection()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
    }
  }
}

int getDhtReading() 
{
  currentTemperature = dht.readTemperature();
  currentHumidity = dht.readHumidity();
  if (isnan(currentTemperature) || isnan(currentHumidity))
  {
    return 0;
  }
  else 
  {
    return 1;
  }
}

float absoluteValue(float value) 
{
  return value < 0 ? -value : value;
}

void sendPacket(const char *str, IPAddress remoteIP, unsigned int remotePort)
{
  udp.beginPacket(remoteIP, remotePort);
  udp.write((uint8_t *)str, strlen(str));
  udp.endPacket();
  Serial.printf("Sent a UDP packet to %s, port %d\n", remoteIP.toString().c_str(), remotePort);
}

void receivePacket()
{
  memset(incomingPacket, 0, sizeof(incomingPacket));
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    int len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    Serial.printf("Received %d bytes from %s:%d - %s\n", packetSize, sendAddress, sendPort, incomingPacket);
  }
  else
  {
    incomingPacket[0] = '\0';
  }
}