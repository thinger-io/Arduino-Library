#include <SPI.h>
#include <Ethernet.h>
#include <ThingerEthernet.h>

ThingerEthernet arduino("USERNAME", "DEVICE", "CREDENTIAL");
const int ledPin =  13;

void setup() {
  pinMode(13, OUTPUT);
  arduino["led"].in() = [](pson& in){ digitalWrite(ledPin, in ? HIGH :LOW); };
}

void loop() {
  arduino.handle();
}