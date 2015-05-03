#ifndef __CC3200R1M1RGC__
#include <SPI.h>
#endif
#include <WiFi.h>
#include <ThingerWifi.h>

ThingerWifi cc3200("USERNAME", "DEVICE", "CREDENTIAL");
const int ledPin =  13;

void setup() {
  pinMode(ledPin, OUTPUT);
  cc3200.add_wifi("SSID", "SSID_PASSWORD");
  cc3200["led"].in() = [](pson& in){ digitalWrite(ledPin, in ? HIGH :LOW); };
}

void loop() {
  cc3200.handle();
}