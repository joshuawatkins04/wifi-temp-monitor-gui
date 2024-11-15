#include <WiFi.h>
#include <WiFiUdp.h>
#include <DHT.h>
#include "config.h"

#define DHT22_PIN 23

WiFiUDP udp;
DHT dht(DHT22_PIN, DHT22);

float temperature;
float humidity;
int checkValid = 0;

void setup() {
    Serial.begin(9600);
    
    dht.begin();
    delay(2000);

    WiFi.begin(ssid, password);
    udp.begin(sendPort);
}

void loop() {
    checkValid = getTemperature();

    if (checkValid == 1) {
        char temperatureString[8];
        dtostrf(temperature, 6, 2, temperatureString);
        sendPacket(temperatureString);
        Serial.print("Sent temperature: ");
        Serial.println(temperatureString);
    } 
    delay(3000);
}

void sendPacket(char message[]) {
    udp.beginPacket(sendAddress, sendPort);
    udp.print(message);
    udp.endPacket();
}

int getTemperature() {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("### Reading DHT sensor failed! ###");
        return 0;
    } else {
        Serial.print("Temp: ");
        Serial.print(temperature);
        Serial.print(" C ");
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.println("%");
        return 1;
    }
}