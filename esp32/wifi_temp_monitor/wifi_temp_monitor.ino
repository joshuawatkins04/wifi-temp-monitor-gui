#include <WiFi.h>
#include <WiFiUdp.h>
#include <DHT.h>
#include "config.h"

#define DHT22_PIN 23

WiFiUDP udp;
DHT dht(DHT22_PIN, DHT22);

float temperature, humidity, lastTemperature = 0, lastHumidity = 0;
int checkValid = 0;

void setup() {
  // Serial.begin(9600);
    
  dht.begin();
  delay(2000);

  WiFi.begin(ssid, password);
  udp.begin(sendPort);
}

void loop() {
  checkValid = getDhtReading();

  if (checkValid == 1) {
    if (temperature != lastTemperature || humidity != lastHumidity) {
      char temperatureString[6];
      char humidityString[6];
      char packetString[15];

      dtostrf(temperature, 5, 2, temperatureString);
      dtostrf(humidity, 5, 2, humidityString);
      snprintf(packetString, sizeof(packetString), "%s,%s", temperatureString, humidityString);
      sendPacket(packetString);

      // Serial.print("Sent data: ");
      // Serial.println(packetString);

      lastTemperature = temperature;
      lastHumidity = humidity;
    }
    delay(5000);
  }
}

void sendPacket(char message[]) {
  udp.beginPacket(sendAddress, sendPort);
  udp.print(message);
  udp.endPacket();
}

int getDhtReading() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    // Serial.println("### Reading DHT sensor failed! ###");
    return 0;
  } else {
    // Serial.print("Temp: ");
    // Serial.print(temperature);
    // Serial.print(" C ");
    // Serial.print("Humidity: ");
    // Serial.print(humidity);
    // Serial.println("%");
    return 1;
  }
}