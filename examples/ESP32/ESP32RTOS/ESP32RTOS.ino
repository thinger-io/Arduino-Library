#define THINGER_SERIAL_DEBUG
#define THINGER_FREE_RTOS

#include <ThingerESP32.h>
#include "arduino_secrets.h"

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  // open serial for debugging
  Serial.begin(115200);

  pinMode(16, OUTPUT);

  thing.add_wifi(SSID, SSID_PASSWORD);

  // digital pin control example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
  thing["GPIO_16"] << digitalPin(16);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // start thinger on its own task
  thing.start();

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  // use loop as in normal Arduino Sketch
  // use thing.lock() thing.unlock() if using variables exposed on thinger resources
}