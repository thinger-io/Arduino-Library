#define THINGER_SERIAL_DEBUG

#include <ThingerMbed.h>
#include <ThingerMbedOTA.h>
#include "arduino_secrets.h"

ThingerMbed thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
ThingerMbedOTA ota(thing);

// cannot connect? Update WiFiNiNA and add iot.thinger.io SSL Certificate
// https://support.arduino.cc/hc/en-us/articles/360016119219

void setup() {
  // configure LED_BUILTIN for output
  pinMode(LED_BUILTIN, OUTPUT);

  // open serial for debugging
  Serial.begin(115200);

  // configure wifi network
  thing.add_wifi(SSID, SSID_PASSWORD);

  // pin control example (i.e. turning on/off a light, a relay, etc)
  thing["led"] << digitalPin(LED_BUILTIN);

  // resource output example (i.e. reading a sensor value, a variable, etc)
  thing["millis"] >> outputValue(millis());

  // start thinger task
  thing.start();

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  // use loop as in normal Arduino Sketch
  // use thing.lock() thing.unlock() if using variables exposed on thinger resources
}