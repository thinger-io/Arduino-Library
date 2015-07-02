#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <ccspi.h>
#include <ThingerCC3000.h>

ThingerCC3000 thing("USERNAME", "DEVICE", "CREDENTIAL");
const int ledPin =  13;

void setup() {
  pinMode(ledPin, OUTPUT);
  thing.add_wifi("SSID", "SSID_PASSWORD");
  thing["led"] <<= [](pson& in){ digitalWrite(ledPin, in ? HIGH : LOW); };
}

void loop() {
  thing.handle();
}