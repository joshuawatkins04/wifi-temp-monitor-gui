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
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 10000;

void setup() 
{
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
}

void loop() 
{
  if (millis() - lastUpdate >= updateInterval)
  {
    lastUpdate = millis();
    checkWifiConnection();
    if (getDhtReading() == 1)
    {
      if (absoluteValue(currentTemperature - lastTemperature) > 0.1 || absoluteValue(currentHumidity - lastHumidity) > 0.2) 
      {
        udp.beginPacket(sendAddress, sendPort);
        udp.printf("%.2f,%.2f", currentTemperature, currentHumidity);
        udp.endPacket();

        lastTemperature = currentTemperature;
        lastHumidity = currentHumidity;
      }
    }
  }
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