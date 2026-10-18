// Raspberry Pi Pico W / Pico 2 W with the Arduino-Pico core:
// https://github.com/earlephilhower/arduino-pico
// For OTA updates select a Flash Size with a file system large enough
// for the firmware (i.e. "2MB (Sketch: 1MB, FS: 1MB)")

#define THINGER_SERIAL_DEBUG

#include <ThingerPicoW.h>
#include <ThingerPicoOTA.h>
#include "arduino_secrets.h"

ThingerPicoW thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

// Initialize Pico OTA
// use Thinger.io VSCode Studio extension + Platformio to upgrade the device remotely
ThingerPicoOTA ota(thing);

void setup() {
  // open serial for monitoring
  Serial.begin(115200);

  // set builtin led as output
  pinMode(LED_BUILTIN, OUTPUT);

  // add WiFi credentials
  thing.add_wifi(SSID, SSID_PASSWORD);
  // add more networks calling add_wifi again: it connects to the strongest one
  // thing.add_wifi("other_ssid", "other_password");

  // digital pin control example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
  thing["led"] << digitalPin(LED_BUILTIN);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}
