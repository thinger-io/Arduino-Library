#include <WiFi.h>
#include <ThingerWifi.h>
#include "arduino_secrets.h"

ThingerWifi thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  // set the boards led to output
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);

  // configure wifi network
  thing.add_wifi(SSID, SSID_PASSWORD);

  // pin control example (i.e. turning on/off a light, a relay, etc)
  thing["led"]["green"] << digitalPin(GREEN_LED);
  thing["led"]["yellow"] << digitalPin(YELLOW_LED);
  thing["led"]["red"] << digitalPin(RED_LED);

  // resource output example (i.e. reading a sensor value, a variable, etc)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}