#define THINGER_SERIAL_DEBUG

#include <ThingerR4WiFi.h>
#include "arduino_secrets.h"

ThingerR4WiFi thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

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