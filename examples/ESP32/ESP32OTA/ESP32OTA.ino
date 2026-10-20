#define THINGER_SERIAL_DEBUG

// compatibility fields of this firmware: the VS Code extension asks for confirmation before updating
// a device with different values (i.e., a firmware for another PCB version)
// #define THINGER_OTA_CHECK(FIELD) FIELD("PCB_MODEL", "MY_BOARD") FIELD("PCB_VERSION", "V1")

// roll back to the previous firmware if a new one cannot connect to Thinger.io in 5 minutes, or reboots before
// #define THINGER_OTA_ROLLBACK

#include <ThingerESP32.h>
#include <ThingerESP32OTA.h>
#include "arduino_secrets.h"

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

// Initialize ESP32OTA OTA
// use Thinger.io VSCode Studio extension + Platformio to upgrade the device remotely
ThingerESP32OTA ota(thing);

void setup() {
  // open serial for debugging
  Serial.begin(115200);

  pinMode(16, OUTPUT);

  thing.add_wifi(SSID, SSID_PASSWORD);

  // digital pin control example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
  thing["GPIO_16"] << digitalPin(16);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}