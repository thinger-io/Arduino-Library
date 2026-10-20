#define THINGER_SERIAL_DEBUG

// Boards with WiFi on the Arduino Zephyr core (Arduino Zephyr Boards), i.e., Portenta H7, Opta or GIGA R1 WiFi.
// Install the Zephyr loader once with Tools > Burn Bootloader. If the WiFi does not start, install the WiFi module
// firmware with the WiFiFirmwareUpdater example of the Arduino Mbed OS core (STM32H747_System library)
#include <ThingerZephyr.h>
#include "arduino_secrets.h"

ThingerZephyr thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  // open serial for debugging
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  // configure wifi network
  thing.add_wifi(SSID, SSID_PASSWORD);

  // pin control example (i.e. turning on/off a light, a relay, etc)
  thing["led"] << digitalPin(LED_BUILTIN);

  // resource output example (i.e. reading a sensor value, a variable, etc)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}
