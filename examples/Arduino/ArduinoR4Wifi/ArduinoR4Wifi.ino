#define THINGER_SERIAL_DEBUG

// compatibility fields of this firmware: the VS Code extension asks for confirmation before updating
// a device with different values (i.e., a firmware for another PCB version)
// #define THINGER_OTA_CHECK(FIELD) FIELD("PCB_MODEL", "MY_BOARD") FIELD("PCB_VERSION", "V1")

#include <ThingerR4WiFi.h>
#include <ThingerR4OTA.h>
#include "arduino_secrets.h"

ThingerR4WiFi thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

// Initialize UNO R4 WiFi OTA (requires the UNO R4 WiFi firmware 0.4.1 or later)
// use Thinger.io VSCode Studio extension + Platformio to upgrade the device remotely
ThingerR4OTA ota(thing);

void setup() {
    // open serial for debugging
    Serial.begin(115200);

    // configure wifi network
    thing.add_wifi(SSID, SSID_PASSWORD);

    pinMode(LED_BUILTIN, OUTPUT);

    // pin control example (i.e. turning on/off a light, a relay, etc)
    thing["led"] << digitalPin(LED_BUILTIN);

    // resource output example (i.e. reading a sensor value, a variable, etc)
    thing["millis"] >> outputValue(millis());

    // more details at http://docs.thinger.io/arduino/
}

void loop() {
    thing.handle();
}