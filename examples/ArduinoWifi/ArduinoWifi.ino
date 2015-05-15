#include <SPI.h>
#include <WiFi.h>
#include <ThingerWifi.h>

ThingerWifi arduino("USERNAME", "DEVICE", "CREDENTIAL");
const int ledPin =  13;

void setup() {
  pinMode(ledPin, OUTPUT);
  arduino.add_wifi("SSID", "SSID_PASSWORD");
  arduino["led"] <<= [](pson& in){ digitalWrite(ledPin, in ? HIGH :LOW); };
}

void loop() {
  arduino.handle();
}